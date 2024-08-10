#pragma once

#include <stdlib.h>
#include <time.h>

#include <box2d/box2d.h>

#include <GLFW/glfw3.h>
#include <chuck/chugin.h>
#include <glfw3webgpu/glfw3webgpu.h>
#include <glm/gtx/string_cast.hpp>

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_wgpu.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // ImPool<>, ImHashData

#include <sokol/sokol_time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include "camera.cpp"
#include "graphics.h"
#include "r_component.h"
#include "sg_command.h"
#include "sg_component.h"
#include "tests/test_base.h"

// necessary for copying from command
static_assert(sizeof(u32) == sizeof(b2WorldId), "b2WorldId != u32");

// Usage:
//  static ImDrawDataSnapshot snapshot; // Important: make persistent accross
//  frames to reuse buffers. snapshot.SnapUsingSwap(ImGui::GetDrawData(),
//  ImGui::GetTime());
//  [...]
//  ImGui_ImplDX11_RenderDrawData(&snapshot.DrawData);
// Source: https://github.com/ocornut/imgui/issues/1860

struct ImDrawDataSnapshotEntry {
    ImDrawList* SrcCopy = NULL; // Drawlist owned by main context
    ImDrawList* OurCopy = NULL; // Our copy
    double LastUsedTime = 0.0;
};

struct ImDrawDataSnapshot {
    // Members
    ImDrawData DrawData;
    ImPool<ImDrawDataSnapshotEntry> Cache;
    float MemoryCompactTimer = 20.0f; // Discard unused data after 20 seconds

    // Functions
    ~ImDrawDataSnapshot()
    {
        Clear();
    }
    void Clear();
    void SnapUsingSwap(ImDrawData* src,
                       double current_time); // Efficient snapshot by swapping data,
                                             // meaning "src_list" is unusable.
    // void                          SnapUsingCopy(ImDrawData* src, double
    // current_time); // Deep-copy snapshop

    // Internals
    ImGuiID GetDrawListID(ImDrawList* src_list)
    {
        return ImHashData(&src_list, sizeof(src_list));
    } // Hash pointer
    ImDrawDataSnapshotEntry* GetOrAddEntry(ImDrawList* src_list)
    {
        return Cache.GetOrAddByKey(GetDrawListID(src_list));
    }
};

void ImDrawDataSnapshot::Clear()
{
    for (int n = 0; n < Cache.GetMapSize(); n++)
        if (ImDrawDataSnapshotEntry* entry = Cache.TryGetMapData(n))
            IM_DELETE(entry->OurCopy);
    Cache.Clear();
    DrawData.Clear();
}

void ImDrawDataSnapshot::SnapUsingSwap(ImDrawData* src, double current_time)
{
    ImDrawData* dst = &DrawData;
    IM_ASSERT(src != dst && src->Valid);

    // Copy all fields except CmdLists[]
    ImVector<ImDrawList*> backup_draw_list;
    backup_draw_list.swap(src->CmdLists);
    IM_ASSERT(src->CmdLists.Data == NULL);
    *dst = *src;
    backup_draw_list.swap(src->CmdLists);

    // Swap and mark as used
    for (ImDrawList* src_list : src->CmdLists) {
        ImDrawDataSnapshotEntry* entry = GetOrAddEntry(src_list);
        if (entry->OurCopy == NULL) {
            entry->SrcCopy = src_list;
            entry->OurCopy = IM_NEW(ImDrawList)(src_list->_Data);
        }
        IM_ASSERT(entry->SrcCopy == src_list);
        entry->SrcCopy->CmdBuffer.swap(entry->OurCopy->CmdBuffer); // Cheap swap
        entry->SrcCopy->IdxBuffer.swap(entry->OurCopy->IdxBuffer);
        entry->SrcCopy->VtxBuffer.swap(entry->OurCopy->VtxBuffer);
        entry->SrcCopy->CmdBuffer.reserve(
          entry->OurCopy->CmdBuffer.Capacity); // Preserve bigger size to avoid reallocs
                                               // for two consecutive frames
        entry->SrcCopy->IdxBuffer.reserve(entry->OurCopy->IdxBuffer.Capacity);
        entry->SrcCopy->VtxBuffer.reserve(entry->OurCopy->VtxBuffer.Capacity);
        entry->LastUsedTime = current_time;
        dst->CmdLists.push_back(entry->OurCopy);
    }

    // Cleanup unused data
    const double gc_threshold = current_time - MemoryCompactTimer;
    for (int n = 0; n < Cache.GetMapSize(); n++)
        if (ImDrawDataSnapshotEntry* entry = Cache.TryGetMapData(n)) {
            if (entry->LastUsedTime > gc_threshold) continue;
            IM_DELETE(entry->OurCopy);
            Cache.Remove(GetDrawListID(entry->SrcCopy), entry);
        }
};

static ImDrawDataSnapshot snapshot;

struct TickStats {
    u64 fc    = 0;
    u64 min   = UINT64_MAX;
    u64 max   = 0;
    u64 total = 0;

    void update(u64 ticks)
    {
        min = ticks < min ? ticks : min;
        max = ticks > max ? ticks : max;
        total += ticks;
        if (++fc % 60 == 0) {
            print("");
            // fc    = 0;
            // total = 0;
        }
    }

    void print(const char* name)
    {
        printf("%s: min: %f, max: %f, avg: %f\n", name, stm_ms(min), stm_ms(max),
               stm_ms(total / fc));
    }
};

TickStats critical_section_stats = {};

static int mini(int x, int y)
{
    return x < y ? x : y;
}

static int maxi(int x, int y)
{
    return x > y ? x : y;
}

GLFWmonitor* getCurrentMonitor(GLFWwindow* window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor* bestmonitor;
    GLFWmonitor** monitors;
    const GLFWvidmode* mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++) {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap = maxi(0, mini(wx + ww, mx + mw) - maxi(wx, mx))
                  * maxi(0, mini(wy + wh, my + mh) - maxi(wy, my));

        if (bestoverlap < overlap) {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}

struct App;
static void _R_HandleCommand(App* app, SG_Command* command);

static void _R_RenderScene(App* app, WGPURenderPassEncoder renderPass);

static void _R_glfwErrorCallback(int error, const char* description)
{
    log_error("GLFW Error[%i]: %s\n", error, description);
}

static int frame_buffer_width  = 0;
static int frame_buffer_height = 0;
struct App {
    // options
    bool standalone; // no chuck. renderer only

    GLFWwindow* window;
    GraphicsContext gctx; // pass as pointer?

    // Chuck Context
    Chuck_VM* ckvm;
    CK_DL_API ckapi;

    // frame metrics
    u64 fc;
    f64 lastTime;
    f64 dt;
    bool show_fps_title = true;

    // renderer tests
    Camera camera;
    TestCallbacks callbacks;

    // scenegraph state
    SG_ID mainScene;

    // imgui
    bool imgui_disabled = false;

    // box2D physics
    b2WorldId b2_world_id;
    u32 b2_substep_count = 4;
    // f64 b2_time_step_accum  = 0;

    // memory
    Arena frameArena;

    // ============================================================================
    // App API
    // ============================================================================

    static void init(App* app, Chuck_VM* vm, CK_DL_API api)
    {
        ASSERT(app->ckvm == NULL && app->ckapi == NULL);
        ASSERT(app->window == NULL);

        app->ckvm  = vm;
        app->ckapi = api;

        Camera::init(&app->camera);
        Arena::init(&app->frameArena, MEGABYTE); // 1MB
    }

    static void gameloop(App* app)
    {

        // frame metrics ----------------------------
        {
            _calculateFPS(app->window, app->show_fps_title);

            ++app->fc;
            f64 currentTime = glfwGetTime();

            // first frame prevent huge dt
            if (app->lastTime == 0) app->lastTime = currentTime;

            app->dt       = currentTime - app->lastTime;
            app->lastTime = currentTime;
        }

        // handle window resize (special case b/c needs to happen before
        // GraphicsContext::prepareFrame, unlike the rest of glfwPollEvents())
        // Doing window resize AFTER surface is already prepared causes crash.
        // Normally you glfwPollEvents() at the start of the frame, but
        // dearImGUI hooks into glfwPollEvents, and modifies imgui state, so
        // glfwPollEvents() must happen in the critial region, after
        // GraphicsContext::prepareFrame
        {
            int width, height;
            glfwGetFramebufferSize(app->window, &width, &height);
            if (width != frame_buffer_width || height != frame_buffer_height) {
                frame_buffer_width  = width;
                frame_buffer_height = height;
                _onFramebufferResize(app->window, width, height);
            }
        }

        if (app->standalone)
            _testLoop(app); // renderer-only tests
        else
            _mainLoop(app); // chuck loop

        Arena::clear(&app->frameArena);
    }

    static void emscriptenMainLoop(void* arg)
    {
        App* app = (App*)arg;
        gameloop(app);
    }

    static void start(App* app)
    {
        ASSERT(app->window == NULL);

        // seed random number generator ===========================
        srand((unsigned int)time(0));

        { // Initialize window
            if (!glfwInit()) {
                log_fatal("Failed to initialize GLFW\n");
                return;
            }

            // Create the window without an OpenGL context
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            t_CKVEC2 window_size = CHUGL_Window_WindowSize();
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER,
                           CHUGL_Window_Transparent() ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_DECORATED,
                           CHUGL_Window_Decorated() ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_RESIZABLE,
                           CHUGL_Window_Resizable() ? GLFW_TRUE : GLFW_FALSE);
            glfwWindowHint(GLFW_FLOATING,
                           CHUGL_Window_Floating() ? GLFW_TRUE : GLFW_FALSE);

            app->window = glfwCreateWindow((int)window_size.x, (int)window_size.y,
                                           "ChuGL", NULL, NULL);

            // TODO: set window user pointer to CHUGL_App

            if (!app->window) {
                log_fatal("Failed to create GLFW window\n");
                glfwTerminate();
                return;
            }
        }

        // init graphics context
        if (!GraphicsContext::init(&app->gctx, app->window)) {
            log_fatal("Failed to initialize graphics context\n");
            return;
        }

        // initialize R_Component manager
        Component_Init(&app->gctx);

        { // set window callbacks
#ifdef CHUGL_DEBUG
            glfwSetErrorCallback(_R_glfwErrorCallback);
#endif
            glfwSetWindowUserPointer(app->window, app);
            glfwSetMouseButtonCallback(app->window, _mouseButtonCallback);
            glfwSetScrollCallback(app->window, _scrollCallback);
            glfwSetCursorPosCallback(app->window, _cursorPositionCallback);
            glfwSetKeyCallback(app->window, _keyCallback);
            glfwSetWindowCloseCallback(app->window, _closeCallback);
            glfwSetWindowContentScaleCallback(app->window, _contentScaleCallback);
            // set initial content scale
            float content_scale_x, content_scale_y;
            glfwGetWindowContentScale(app->window, &content_scale_x, &content_scale_y);
            CHUGL_Window_ContentScale(content_scale_x, content_scale_y);

            glfwPollEvents(); // call poll events first to get correct
                              //   framebuffer size (glfw bug:
                              //   https://github.com/glfw/glfw/issues/1968)
        }

        { // initialize imgui
            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void)io;
            io.ConfigFlags
              |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
            io.ConfigFlags
              |= ImGuiConfigFlags_NavEnableGamepad;           // Enable Gamepad Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            // ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOther(app->window, true);
#ifdef __EMSCRIPTEN__
            ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
            ImGui_ImplWGPU_InitInfo init_info;
            init_info.Device             = app->gctx.device;
            init_info.NumFramesInFlight  = 3;
            init_info.RenderTargetFormat = app->gctx.swapChainFormat;
            init_info.DepthStencilFormat = app->gctx.depthTextureDesc.format;
            ImGui_ImplWGPU_Init(&init_info);
        }

        // trigger window resize callback to set up imgui
        int width, height;
        glfwGetFramebufferSize(app->window, &width, &height);
        _onFramebufferResize(app->window, width, height);

        // initialize imgui frame (should be threadsafe as long as graphics
        // shreds start with GG.nextFrame() => now)
        if (!app->standalone) {
            ImGui_ImplWGPU_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        if (app->standalone && app->callbacks.onInit)
            app->callbacks.onInit(&app->gctx, app->window);

        app->camera.entity.pos = glm::vec3(0.0, 0.0, 6.0); // move camera back

        // main loop
        log_trace("entering  main loop");
#ifdef __EMSCRIPTEN__
        // https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop_arg
        // can't have an infinite loop in emscripten
        // instead pass a callback to emscripten_set_main_loop_arg
        emscripten_set_main_loop_arg(
          [](void* runner) {
              App::emscriptenMainLoop(runner);
              //   gameloop(app);
              // if (glfwWindowShouldClose(app->window)) {
              //     if (app->callbacks.onExit) app->callbacks.onExit();
              //     emscripten_cancel_main_loop(); // unregister the main loop
              // }
          },
          app, // user data (void *)
          -1,  // FPS (negative means use browser's requestAnimationFrame)
          true // simulate infinite loop (prevents code after this from exiting)
        );
#else
        while (!glfwWindowShouldClose(app->window)) gameloop(app);
#endif

        log_trace("Exiting main loop");
        if (app->standalone && app->callbacks.onExit) app->callbacks.onExit();
    }

    static void end(App* app)
    {
        // free R_Components
        Component_Free();

        // release graphics context
        GraphicsContext::release(&app->gctx);

        // destroy imgui
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // destroy window
        glfwDestroyWindow(app->window);

        // terminate GLFW
        glfwTerminate();

        // free memory
        Arena::free(&app->frameArena);

        // zero all fields
        *app = {};
    }

    // ============================================================================
    // App Internal Functions
    // ============================================================================

    static void _testLoop(App* app)
    {
        // handle input -------------------
        glfwPollEvents();
        // update camera
        // TODO store aspect in app state
        i32 width, height;
        glfwGetWindowSize(app->window, &width, &height);
        f32 aspect = (f32)width / (f32)height;
        app->camera.update(&app->camera, 1.0f / 60.0f); // TODO actually set dt

        if (app->callbacks.onUpdate) app->callbacks.onUpdate(app->dt);
        if (app->callbacks.onRender) {
            app->callbacks.onRender(Camera::projectionMatrix(&app->camera, aspect),
                                    Entity::viewMatrix(&app->camera.entity),
                                    app->camera.entity.pos);
        }
    }

    static void _mainLoop(App* app)
    {
        ASSERT(!app->standalone);
        // Render initialization ==================================
        // TODO

        // Copy from CGL scenegraph ====================================
        // TODO do in main hook

        // fps
        // const t_CKUINT N = 30;
        // double deltaTimes[N];
        // double deltaSum = 0;
        // memset(deltaTimes, 0, sizeof(deltaTimes));
        // t_CKUINT dtWrite      = 0;
        // t_CKUINT dtInitFrames = 0;
        // bool firstFrame       = true;

        // must happen after window resize

        // Render Loop ===========================================
        static u64 prev_lap_time{ stm_now() };

        // ======================
        // enter critical section
        // ======================
        // waiting for audio synchronization (see cgl_update_event_waiting_on)
        // (i.e., when all registered GG.nextFrame() are called on their
        // respective shreds)
        Sync_WaitOnUpdateDone();
        // log_error("graphics thread: all shreds have waited");

        // question: why does putting this AFTER time calculation cause
        // everything to be so choppy at high FPS? hypothesis: puts time
        // calculation into the critical region time is updated only when all
        // chuck shreds are on wait queue guaranteeing that when they awake,
        // they'll be using fresh dt data

        { // calculate dt
            u64 dt_ticks = stm_laptime(&prev_lap_time);
            f64 dt_sec   = stm_sec(dt_ticks);
            CHUGL_Window_dt(dt_sec);
        }

        /* two locks here:
        1 for writing/swapping the command queues
            - this lock is grabbed by chuck every time we do a CGL call
            - supports writing CGL commands whenever, even outside game loop
        1 for the condition_var used to synchronize audio and graphics each
        frame
            - combined with the chuck-side update_event, allows for writing
        frame-accurate cgl commands
            - exposes a gameloop to chuck, gauranteed to be executed once per
        frame deadlock shouldn't happen because both locks are never held at the
        same time */
        bool do_ui = !app->imgui_disabled;

        {
            CQ_SwapQueues(); // ~ .0001ms

            // u64 critical_start = stm_now();
            // Rendering
            if (do_ui) {
                ImGui::Render();
                // log_error("graphics thread: imgui render");

                // copy imgui draw data for rendering later
                snapshot.SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());
            }

            // imgui and window callbacks
            CHUGL_Zero_Mouse_Deltas();
            glfwPollEvents();

            if (do_ui) {
                // reset imgui
                ImGui_ImplWGPU_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                // enable docking to main window
                ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                             ImGuiDockNodeFlags_PassthruCentralNode);
            }
            // ~2.15ms (15%) In DEBUG mode!
            // critical_section_stats.update(stm_since(critical_start));

            // physics
            // TODO: detach timestap from framerate
            // https://gafferongames.com/post/fix_your_timestep/
            if (b2World_IsValid(app->b2_world_id)) {
                b2World_Step(app->b2_world_id, app->dt, app->b2_substep_count);
                log_trace("simulating %d %f", app->b2_world_id.index1, app->dt);
            }
        }

        // done swapping the double buffer, let chuck know it's good to continue
        // pushing commands this wakes all shreds currently waiting on
        // GG.nextFrame()
        // log_error("graphics thread: waking up shreds");

        // grabs waitingShredsLock
        Event_Broadcast(CHUGL_EventType::NEXT_FRAME, app->ckapi, app->ckvm);

        // ====================
        // end critical section
        // ====================

        // now apply changes from the command queue chuck is NO Longer writing
        // to this executes all commands on the command queue, performs actions
        // from CK code essentially applying a diff to bring graphics state up
        // to date with what is done in CK code

        { // flush command queue
            SG_Command* cmd = NULL;
            while (CQ_ReadCommandQueueIter(&cmd)) _R_HandleCommand(app, cmd);
            CQ_ReadCommandQueueClear();
            // tasks to do after command queue is flushed (batched)
            Material_batchUpdatePipelines(&app->gctx);
        }

        // process any glfw options passed from chuck
        // UpdateState(scene);

        // garbage collection! delete GPU-side data for any scenegraph objects
        // that were deleted in chuck
        // renderer.ProcessDeletionQueue(
        //   &scene); // IMPORTANT: should happen after flushing command queue

        // now renderer can work on drawing the copied scenegraph
        // renderer.RenderScene(&scene, scene.GetMainCamera());

        GraphicsContext::prepareFrame(&app->gctx);

        WGPURenderPassEncoder render_pass = NULL;
        { // render pass
            render_pass = wgpuCommandEncoderBeginRenderPass(app->gctx.commandEncoder,
                                                            &app->gctx.renderPassDesc);

            // scene
            _R_RenderScene(app, render_pass);

            // UI
            if (do_ui) ImGui_ImplWGPU_RenderDrawData(&snapshot.DrawData, render_pass);

            wgpuRenderPassEncoderEnd(render_pass);
        }

        // submit and present
        GraphicsContext::presentFrame(&app->gctx);

        // cleanup
        wgpuRenderPassEncoderRelease(render_pass);
    }

    static void _calculateFPS(GLFWwindow* window, bool print_to_title)
    {
#define WINDOW_TITLE_MAX_LENGTH 256

        static f64 lastTime{ glfwGetTime() };
        static u64 frameCount{};
        static char title[WINDOW_TITLE_MAX_LENGTH]{};

        // Measure speed
        f64 currentTime = glfwGetTime();
        f64 delta       = currentTime - lastTime;
        frameCount++;
        if (delta >= 1.0) { // If last cout was more than 1 sec ago
            f64 fps = frameCount / delta;
            CHUGL_Window_fps(fps);
            if (print_to_title) {
                snprintf(title, WINDOW_TITLE_MAX_LENGTH, "ChuGL-WebGPU [FPS: %.2f]",
                         fps);
                glfwSetWindowTitle(window, title);
            }

            frameCount = 0;
            lastTime   = currentTime;
        }
#undef WINDOW_TITLE_MAX_LENGTH
    }

    static void _closeCallback(GLFWwindow* window)
    {
        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onExit) app->callbacks.onExit();

        log_error("closing window");

        // ChuGL
        if (!app->standalone) {
            // broadcast WindowCloseEvent
            Event_Broadcast(CHUGL_EventType::WINDOW_CLOSE, app->ckapi, app->ckvm);
            // block closeable
            if (!CHUGL_Window_Closeable()) glfwSetWindowShouldClose(window, GLFW_FALSE);
        }
    }

    static void _contentScaleCallback(GLFWwindow* window, float xscale, float yscale)
    {
        App* app = (App*)glfwGetWindowUserPointer(window);

        // broadcast to chuck
        if (!app->standalone) {
            CHUGL_Window_ContentScale(xscale, yscale);
            // update content scale
            Event_Broadcast(CHUGL_EventType::CONTENT_SCALE, app->ckapi, app->ckvm);
        }
    }

    static void _keyCallback(GLFWwindow* window, int key, int scancode, int action,
                             int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS
            && CHUGL_Window_Closeable()) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }

        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onKey) app->callbacks.onKey(key, scancode, action, mods);
    }

    // this is deliberately NOT made a glfw callback because glfwPollEvents()
    // happens AFTER GraphicsContext::PrepareFrame(), after render surface has
    // already been set window resize needs to be handled before the frame is
    // prepared
    static void _onFramebufferResize(GLFWwindow* window, int width, int height)
    {
        log_trace("window resized: %d, %d", width, height);

        App* app = (App*)glfwGetWindowUserPointer(window);

        // causes inconsistent crash on window resize
        // ImGui_ImplWGPU_InvalidateDeviceObjects();

        GraphicsContext::resize(&app->gctx, width, height);

        // ImGui_ImplWGPU_CreateDeviceObjects();

        if (app->callbacks.onWindowResize) app->callbacks.onWindowResize(width, height);

        if (!app->standalone) {
            // update size stats
            int window_width, window_height;
            glfwGetWindowSize(window, &window_width, &window_height);
            CHUGL_Window_Size(window_width, window_height, width, height);
            // broadcast to chuck
            Event_Broadcast(CHUGL_EventType::WINDOW_RESIZE, app->ckapi, app->ckvm);
        }
    }

    static void _mouseButtonCallback(GLFWwindow* window, int button, int action,
                                     int mods)
    {
        // log_debug("mouse button callback");

        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;

        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onMouseButton)
            app->callbacks.onMouseButton(button, action, mods);

        app->camera.onMouseButton(&app->camera, button, action, mods);
    }

    static void _scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;

        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onScroll) app->callbacks.onScroll(xoffset, yoffset);

        app->camera.onScroll(&app->camera, xoffset, yoffset);
    }

    static void _cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
    {
        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onCursorPosition)
            app->callbacks.onCursorPosition(xpos, ypos);

        app->camera.onCursorPosition(&app->camera, xpos, ypos);

        CHUGL_Mouse_Position(xpos, ypos);
    }
};

static void _R_RenderScene(App* app, WGPURenderPassEncoder renderPass)
{
    // early out if no pipelines
    if (Component_RenderPipelineCount() == 0) return;

    R_Scene* main_scene = Component_GetScene(app->mainScene);

    { // Update all transforms
        R_Transform::rebuildMatrices(main_scene, &app->frameArena);
        // R_Transform::print(main_scene, 0);
    }

    // update camera
    // TODO switch to GCamera
    i32 width, height;
    glfwGetWindowSize(app->window, &width, &height);
    f32 aspect = (f32)width / (f32)height;
    app->camera.update(&app->camera, 1.0f / 60.0f); // TODO actually set dt

    // write per-frame uniforms
    f32 time                    = (f32)glfwGetTime();
    FrameUniforms frameUniforms = {};
    frameUniforms.projectionMat = Camera::projectionMatrix(&app->camera, aspect);
    frameUniforms.viewMat       = Entity::viewMatrix(&app->camera.entity);
    frameUniforms.projViewMat   = frameUniforms.projectionMat * frameUniforms.viewMat;
    frameUniforms.camPos        = app->camera.entity.pos;
    frameUniforms.dirLight      = VEC_FORWARD;
    frameUniforms.time          = time;

    // update frame-level uniforms
    bool frame_uniforms_recreated = GPU_Buffer::write(
      &app->gctx, &R_RenderPipeline::frame_uniform_buffer, WGPUBufferUsage_Uniform,
      &frameUniforms, sizeof(frameUniforms));
    ASSERT(!frame_uniforms_recreated);

    // log_debug("geo num instances: %d", R_Geometry::numInstances(geo));
    // Test render loop
    R_RenderPipeline* renderPipeline = NULL;
    size_t rpIndex                   = 0;
    while (Component_RenderPipelineIter(&rpIndex, &renderPipeline)) {
        // log_trace("drawing materials for render pipeline: %d",
        //           renderPipeline->rid);

        // early out if numMaterials == 0;
        if (R_RenderPipeline::numMaterials(renderPipeline) == 0) continue;

        WGPURenderPipeline gpu_pipeline = renderPipeline->gpu_pipeline;

        // ==optimize== cache layouts in R_RenderPipeline struct upon creation
        WGPUBindGroupLayout perMaterialLayout
          = wgpuRenderPipelineGetBindGroupLayout(gpu_pipeline, PER_MATERIAL_GROUP);
        WGPUBindGroupLayout perDrawLayout
          = wgpuRenderPipelineGetBindGroupLayout(gpu_pipeline, PER_DRAW_GROUP);
        // WGPUBindGroupLayout vertex_pulling_layout
        //   = wgpuRenderPipelineGetBindGroupLayout(gpu_pipeline, VERTEX_PULL_GROUP);

        // set shader
        // TODO only set shader if we actually have anything to render
        wgpuRenderPassEncoderSetPipeline(renderPass, gpu_pipeline);

        wgpuRenderPassEncoderSetBindGroup(renderPass, PER_FRAME_GROUP,
                                          renderPipeline->frame_group, 0, NULL);

        // per-material render loop
        size_t materialIdx    = 0;
        R_Material* rMaterial = NULL;
        while (
          R_RenderPipeline::materialIter(renderPipeline, &materialIdx, &rMaterial)) {
            // get material
            // log_trace("drawing material: %d", rMaterial->id);

            ASSERT(rMaterial && rMaterial->pipelineID == renderPipeline->rid);

            // early out if numPrimitives == 0;
            if (R_Material::numPrimitives(rMaterial) == 0) continue;

            // TODO: figure out textures / texture views...

            // set per_material bind group
            R_Material::rebuildBindGroup(rMaterial, &app->gctx, perMaterialLayout);
            wgpuRenderPassEncoderSetBindGroup(renderPass, PER_MATERIAL_GROUP,
                                              rMaterial->bind_group, 0, NULL);

            // iterate over material primitives
            size_t primitiveIdx           = 0;
            Material_Primitive* primitive = NULL;
            while (R_Material::primitiveIter(rMaterial, &primitiveIdx, &primitive)) {
                u32 numInstances = Material_Primitive::numInstances(primitive);
                if (numInstances == 0) continue;

                Material_Primitive::rebuildBindGroup(&app->gctx, primitive,
                                                     perDrawLayout, &app->frameArena);

                // set model bind group
                wgpuRenderPassEncoderSetBindGroup(renderPass, PER_DRAW_GROUP,
                                                  primitive->bindGroup, 0, NULL);

                R_Geometry* geo = Component_GetGeometry(primitive->geoID);
                ASSERT(geo);

                // set vertex attributes
                for (int location = 0; location < R_Geometry::vertexAttributeCount(geo);
                     location++) {
                    GPU_Buffer* gpu_buffer = &geo->gpu_vertex_buffers[location];
                    wgpuRenderPassEncoderSetVertexBuffer(
                      renderPass, location, gpu_buffer->buf, 0, gpu_buffer->size);
                }

                // set pulled vertex buffers (programmable vertex pulling)
                // TODO cache layout in outer loop
                if (R_Geometry::usesVertexPulling(geo)) {
                    WGPUBindGroupLayout vertex_pulling_layout
                      = wgpuRenderPipelineGetBindGroupLayout(gpu_pipeline,
                                                             VERTEX_PULL_GROUP);
                    R_Geometry::rebuildPullBindGroup(&app->gctx, geo,
                                                     vertex_pulling_layout);
                    wgpuRenderPassEncoderSetBindGroup(renderPass, VERTEX_PULL_GROUP,
                                                      geo->pull_bind_group, 0, NULL);
                }

                // populate index buffer
                u32 num_indices = R_Geometry::indexCount(geo);
                if (num_indices > 0) {
                    // indexed draw
                    wgpuRenderPassEncoderSetIndexBuffer(
                      renderPass, geo->gpu_index_buffer.buf, WGPUIndexFormat_Uint32, 0,
                      geo->gpu_index_buffer.size);

                    wgpuRenderPassEncoderDrawIndexed(renderPass, num_indices,
                                                     numInstances, 0, 0, 0);
                } else {
                    // non-index draw
                    int num_vertices = (int)R_Geometry::vertexCount(geo);
                    int vertex_draw_count
                      = geo->vertex_count >= 0 ? geo->vertex_count : num_vertices;
                    wgpuRenderPassEncoderDraw(renderPass, vertex_draw_count,
                                              numInstances, 0, 0);
                }
            }
        }
    }
}

static void _R_HandleCommand(App* app, SG_Command* command)
{
    switch (command->type) {
        case SG_COMMAND_WINDOW_CLOSE: {
            glfwSetWindowShouldClose(app->window, GLFW_TRUE);
            break;
        }
        case SG_COMMAND_WINDOW_MODE: {
            SG_Command_WindowMode* cmd = (SG_Command_WindowMode*)command;
            switch (cmd->mode) {
                case SG_WINDOW_MODE_FULLSCREEN: {
                    GLFWmonitor* monitor    = getCurrentMonitor(app->window);
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(app->window, monitor, 0, 0, mode->width,
                                         mode->height, mode->refreshRate);
                    // set fullscreen resolution if specified
                    if (cmd->height > 0 && cmd->width > 0) {
                        glfwSetWindowSize(app->window, cmd->width, cmd->height);
                    }
                    break;
                }
                case SG_WINDOW_MODE_WINDOWED: {
                    // get previous position
                    int xpos, ypos;
                    glfwGetWindowPos(app->window, &xpos, &ypos);
                    glfwSetWindowMonitor(app->window, NULL, xpos, ypos, cmd->width,
                                         cmd->height, GLFW_DONT_CARE);
                    break;
                }
                case SG_WINDOW_MODE_WINDOWED_FULLSCREEN: {
                    GLFWmonitor* monitor    = getCurrentMonitor(app->window);
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    int mx, my;
                    glfwGetMonitorPos(monitor, &mx, &my);
                    glfwSetWindowMonitor(app->window, NULL, mx, my, mode->width,
                                         mode->height, GLFW_DONT_CARE);
                    break;
                }
            }
            break;
        }
        case SG_COMMAND_WINDOW_SIZE_LIMITS: {
            SG_Command_WindowSizeLimits* cmd = (SG_Command_WindowSizeLimits*)command;
            glfwSetWindowSizeLimits(
              app->window, (cmd->min_width <= 0) ? GLFW_DONT_CARE : cmd->min_width,
              (cmd->min_height <= 0) ? GLFW_DONT_CARE : cmd->min_height,
              (cmd->max_width <= 0) ? GLFW_DONT_CARE : cmd->max_width,
              (cmd->max_height <= 0) ? GLFW_DONT_CARE : cmd->max_height);
            glfwSetWindowAspectRatio(
              app->window,
              (cmd->aspect_ratio_x <= 0) ? GLFW_DONT_CARE : cmd->aspect_ratio_x,
              (cmd->aspect_ratio_y <= 0) ? GLFW_DONT_CARE : cmd->aspect_ratio_y);
            // reset size to constrain to new limits
            int width, height;
            glfwGetWindowSize(app->window, &width, &height);
            glfwSetWindowSize(app->window, width, height);
            break;
        }
        case SG_COMMAND_WINDOW_POSITION: {
            SG_Command_WindowPosition* cmd = (SG_Command_WindowPosition*)command;
            // set relative to currenet monitor
            GLFWmonitor* monitor = getCurrentMonitor(app->window);
            int mx, my;
            glfwGetMonitorPos(monitor, &mx, &my);
            glfwSetWindowPos(app->window, mx + cmd->x, my + cmd->y);
            break;
        }
        case SG_COMMAND_WINDOW_CENTER: {
            // center window on current monitor
            GLFWmonitor* monitor    = getCurrentMonitor(app->window);
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            int mx, my;
            glfwGetMonitorPos(monitor, &mx, &my);
            int wx, wy;
            glfwGetWindowSize(app->window, &wx, &wy);
            int xpos = mx + (mode->width - wx) / 2;
            int ypos = my + (mode->height - wy) / 2;
            glfwSetWindowPos(app->window, xpos, ypos);
            break;
        }
        case SG_COMMAND_WINDOW_TITLE: {
            SG_Command_WindowTitle* cmd = (SG_Command_WindowTitle*)command;
            glfwSetWindowTitle(app->window,
                               (char*)CQ_ReadCommandGetOffset(cmd->title_offset));
            app->show_fps_title = false; // disable default FPS title
            break;
        }
        case SG_COMMAND_WINDOW_ICONIFY: {
            SG_Command_WindowIconify* cmd = (SG_Command_WindowIconify*)command;
            if (cmd->iconify)
                glfwIconifyWindow(app->window);
            else
                glfwRestoreWindow(app->window);
            break;
        }
        case SG_COMMAND_WINDOW_ATTRIBUTE: {
            ASSERT(false); // not implemented, changing window attributes after
                           // window creation on macOS causes window to
                           // disappear and freeze
            // SG_Command_WindowAttribute* cmd
            //   = (SG_Command_WindowAttribute*)command;
            // switch (cmd->attrib) {
            //     case CHUGL_WINDOW_ATTRIB_DECORATED:
            //         glfwSetWindowAttrib(app->window, GLFW_DECORATED,
            //                             cmd->value ? GLFW_TRUE : GLFW_FALSE);
            //         break;
            //     case CHUGL_WINDOW_ATTRIB_RESIZABLE:
            //         glfwSetWindowAttrib(app->window, GLFW_RESIZABLE,
            //                             cmd->value ? GLFW_TRUE : GLFW_FALSE);
            //         break;
            //     case CHUGL_WINDOW_ATTRIB_FLOATING:
            //         glfwSetWindowAttrib(app->window, GLFW_FLOATING,
            //                             cmd->value ? GLFW_TRUE : GLFW_FALSE);
            //         break;
            //     default: break;
            // }
        }
        case SG_COMMAND_WINDOW_OPACITY: {
            SG_Command_WindowOpacity* cmd = (SG_Command_WindowOpacity*)command;
            glfwSetWindowOpacity(app->window, cmd->opacity);
            break;
        }
        case SG_COMMAND_MOUSE_MODE: {
            SG_Command_MouseMode* cmd = (SG_Command_MouseMode*)command;
            switch (cmd->mode) {
                case 0:
                    glfwSetInputMode(app->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    break;
                case 1:
                    // TODO doesn't work on macos?
                    glfwSetInputMode(app->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                    break;
                case 2:
                    glfwSetInputMode(app->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    // raw mouse motion
                    if (glfwRawMouseMotionSupported())
                        glfwSetInputMode(app->window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                    break;
            }
            break;
        }
        case SG_COMMAND_MOUSE_CURSOR: {
            SG_Command_MouseCursor* cmd = (SG_Command_MouseCursor*)command;
            if (cmd->mouse_cursor_image_offset == 0 || cmd->width == 0
                || cmd->height == 0) {
                // default to normal cursor
                glfwSetCursor(app->window, NULL);
            } else {
                log_error("setting custom cursor");
                // create cursor
                GLFWimage image;
                image.width  = cmd->width;
                image.height = cmd->height;
                image.pixels = (unsigned char*)CQ_ReadCommandGetOffset(
                  cmd->mouse_cursor_image_offset);

                // print image
                // for (int i = 0; i < image.width * image.height; i++)
                //     printf("%d %d %d %d\n", image.pixels[i * 4],
                //            image.pixels[i * 4 + 1], image.pixels[i * 4 + 2],
                //            image.pixels[i * 4 + 3]);

                GLFWcursor* cursor = glfwCreateCursor(&image, cmd->xhot, cmd->yhot);
                if (!cursor) log_error("failed to create cursor");
                glfwSetCursor(app->window, cursor);

                // static unsigned char pixels[16 * 16 * 4];
                // memset(pixels, 0xff, sizeof(pixels));

                // GLFWimage image;
                // image.width  = 16;
                // image.height = 16;
                // image.pixels = pixels;

                // GLFWcursor* cursor = glfwCreateCursor(&image, 0, 0);
                // log_error("cursor: %p", cursor);
                // glfwSetCursor(app->window, cursor);
            }
            break;
        }
        case SG_COMMAND_MOUSE_CURSOR_NORMAL: {
            log_error("setting normal cursor");
            glfwSetCursor(app->window, NULL);
            break;
        }
        case SG_COMMAND_UI_DISABLED: {
            SG_Command_UI_Disabled* cmd = (SG_Command_UI_Disabled*)command;
            app->imgui_disabled         = cmd->disabled;
            break;
        }
        case SG_COMMAND_GG_SCENE: {
            app->mainScene = ((SG_Command_GG_Scene*)command)->sg_id;
            break;
        }
        case SG_COMMAND_CREATE_XFORM:
            Component_CreateTransform((SG_Command_CreateXform*)command);
            break;
        case SG_COMMAND_ADD_CHILD: {
            SG_Command_AddChild* cmd = (SG_Command_AddChild*)command;
            R_Transform::addChild(Component_GetXform(cmd->parent_id),
                                  Component_GetXform(cmd->child_id));
            break;
        }
        case SG_COMMAND_REMOVE_CHILD: {
            SG_Command_RemoveChild* cmd = (SG_Command_RemoveChild*)command;
            R_Transform::removeChild(Component_GetXform(cmd->parent),
                                     Component_GetXform(cmd->child));
            break;
        }
        case SG_COMMAND_SET_POSITION: {
            SG_Command_SetPosition* cmd = (SG_Command_SetPosition*)command;
            R_Transform::pos(Component_GetXform(cmd->sg_id), cmd->pos);
            break;
        }
        case SG_COMMAND_SET_ROTATATION: {
            SG_Command_SetRotation* cmd = (SG_Command_SetRotation*)command;
            R_Transform::rot(Component_GetXform(cmd->sg_id), cmd->rot);
            break;
        }
        case SG_COMMAND_SET_SCALE: {
            SG_Command_SetScale* cmd = (SG_Command_SetScale*)command;
            R_Transform::sca(Component_GetXform(cmd->sg_id), cmd->sca);
            break;
        }
        case SG_COMMAND_SCENE_CREATE: {
            SG_Command_SceneCreate* cmd = (SG_Command_SceneCreate*)command;
            Component_CreateScene(cmd);
            break;
        }
        case SG_COMMAND_SCENE_BG_COLOR: {
            SG_Command_SceneBGColor* cmd = (SG_Command_SceneBGColor*)command;
            R_Scene* scene               = Component_GetScene(cmd->sg_id);
            scene->bg_color              = cmd->color;
            break;
        }
        case SG_COMMAND_GEO_CREATE: {
            SG_Command_GeoCreate* cmd = (SG_Command_GeoCreate*)command;
            Component_CreateGeometry(&app->gctx, cmd);
        } break;
        case SG_COMMAND_GEO_SET_VERTEX_ATTRIBUTE: {
            SG_Command_GeoSetVertexAttribute* cmd
              = (SG_Command_GeoSetVertexAttribute*)command;
            R_Geometry* geo = Component_GetGeometry(cmd->sg_id);

            f32* data = (f32*)CQ_ReadCommandGetOffset(cmd->data_offset);

            R_Geometry::setVertexAttribute(&app->gctx, geo, cmd->location,
                                           cmd->num_components, data, cmd->data_len);

        } break;
        case SG_COMMAND_GEO_SET_INDICES: {
            SG_Command_GeoSetIndices* cmd = (SG_Command_GeoSetIndices*)command;
            R_Geometry* geo               = Component_GetGeometry(cmd->sg_id);

            u32* indices = (u32*)CQ_ReadCommandGetOffset(cmd->indices_offset);

            R_Geometry::setIndices(&app->gctx, geo, indices, cmd->index_count);
        } break;
        case SG_COMMAND_GEO_SET_PULLED_VERTEX_ATTRIBUTE: {
            SG_Command_GeometrySetPulledVertexAttribute* cmd
              = (SG_Command_GeometrySetPulledVertexAttribute*)command;
            R_Geometry* geo = Component_GetGeometry(cmd->sg_id);

            void* data = CQ_ReadCommandGetOffset(cmd->data_offset);

            R_Geometry::setPulledVertexAttribute(&app->gctx, geo, cmd->location, data,
                                                 cmd->data_bytes);
        } break;
        case SG_COMMAND_GEO_SET_VERTEX_COUNT: {
            SG_Command_GeometrySetVertexCount* cmd
              = (SG_Command_GeometrySetVertexCount*)command;
            R_Geometry* geo   = Component_GetGeometry(cmd->sg_id);
            geo->vertex_count = cmd->count;
        } break;
        // shaders
        case SG_COMMAND_SHADER_CREATE: {
            SG_Command_ShaderCreate* cmd = (SG_Command_ShaderCreate*)command;
            Component_CreateShader(&app->gctx, cmd);
        } break;
        case SG_COMMAND_MATERIAL_CREATE: {
            SG_Command_MaterialCreate* cmd = (SG_Command_MaterialCreate*)command;
            Component_CreateMaterial(&app->gctx, cmd);
        } break;
        case SG_COMMAND_MATERIAL_UPDATE_PSO: {
            SG_Command_MaterialUpdatePSO* cmd = (SG_Command_MaterialUpdatePSO*)command;
            R_Material* material              = Component_GetMaterial(cmd->sg_id);
            R_Material::updatePSO(&app->gctx, material, &cmd->pso);
        } break;
        case SG_COMMAND_MATERIAL_SET_UNIFORM: {
            SG_Command_MaterialSetUniform* cmd
              = (SG_Command_MaterialSetUniform*)command;
            R_Material* material = Component_GetMaterial(cmd->sg_id);
            SG_MaterialUniformPtrAndSize u_ptr_size
              = SG_MaterialUniform::data(&cmd->uniform);
            R_Material::setBinding(&app->gctx, material, cmd->location, R_BIND_UNIFORM,
                                   u_ptr_size.ptr, u_ptr_size.size);
        } break;
        case SG_COMMAND_MATERIAL_SET_STORAGE_BUFFER: {
            SG_Command_MaterialSetStorageBuffer* cmd
              = (SG_Command_MaterialSetStorageBuffer*)command;
            R_Material* material = Component_GetMaterial(cmd->sg_id);
            void* data           = CQ_ReadCommandGetOffset(cmd->data_offset);
            // TODO: only supports f32 storage buffers currently
            R_Material::setBinding(&app->gctx, material, cmd->location, R_BIND_STORAGE,
                                   data, cmd->data_count * sizeof(f32));
        } break;
        case SG_COMMAND_MESH_CREATE: {
            SG_Command_Mesh_Create* cmd = (SG_Command_Mesh_Create*)command;
            Component_CreateMesh(cmd);
            break;
        }
        // b2
        case SG_COMMAND_b2_WORLD_SET: {
            SG_Command_b2World_Set* cmd = (SG_Command_b2World_Set*)command;
            app->b2_world_id            = *(b2WorldId*)&cmd->b2_world_id;
            break;
        }
        case SG_COMMAND_b2_SUBSTEP_COUNT: {
            SG_Command_b2_SubstepCount* cmd = (SG_Command_b2_SubstepCount*)command;
            app->b2_substep_count           = cmd->substep_count;
            break;
        }
        default: ASSERT(false);
    }
}
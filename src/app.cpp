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

#include "core/hashmap.h"

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

// map entry for map from (pipeline_id, camera_id) --> WGPUBindGroup
struct FrameUniformsItem {
    SG_ID pipeline_id;
    SG_ID camera_id;

    // bindgroup never has to be rebuilt, because it points to
    // R_Camera::frame_uniform_buffer which is fixed size of FrameUniforms
    WGPUBindGroup frame_bindgroup;

    static WGPUBindGroup set(hashmap* map, GraphicsContext* gctx,
                             R_RenderPipeline* pipeline, R_Camera* camera)
    {
        FrameUniformsItem item = {};
        item.pipeline_id       = pipeline->rid;
        item.camera_id         = camera->id;

        // create the bindgroup lazily
        WGPUBindGroupEntry frame_group_entry = {};
        frame_group_entry.binding            = 0;
        frame_group_entry.buffer             = camera->frame_uniform_buffer.buf;
        frame_group_entry.size               = camera->frame_uniform_buffer.size;

        // create bind group
        WGPUBindGroupDescriptor frameGroupDesc;
        frameGroupDesc        = {};
        frameGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(
          pipeline->gpu_pipeline, PER_FRAME_GROUP);
        frameGroupDesc.entries    = &frame_group_entry;
        frameGroupDesc.entryCount = 1;

        // layout:auto requires a bind group per pipeline
        item.frame_bindgroup = wgpuDeviceCreateBindGroup(gctx->device, &frameGroupDesc);
        ASSERT(item.frame_bindgroup);

        const void* replaced = hashmap_set(map, &item);
        ASSERT(!replaced);

        return item.frame_bindgroup;
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        FrameUniformsItem* ga = (FrameUniformsItem*)a;
        FrameUniformsItem* gb = (FrameUniformsItem*)b;

        SG_ID key_a[2] = { ga->pipeline_id, ga->camera_id };
        SG_ID key_b[2] = { gb->pipeline_id, gb->camera_id };

        return memcmp(key_a, key_b, sizeof(key_a));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        FrameUniformsItem* e = (FrameUniformsItem*)item;
        SG_ID key[2]         = { e->pipeline_id, e->camera_id };
        return hashmap_xxhash3(&key, sizeof(key), seed0, seed1);
    }

    static void free(void* item)
    {
        FrameUniformsItem* e = (FrameUniformsItem*)item;
        WGPU_RELEASE_RESOURCE(BindGroup, e->frame_bindgroup);
    }
};

static void _R_HandleCommand(App* app, SG_Command* command);

static void _R_RenderScene(App* app, R_Scene* scene, R_Camera* camera,
                           WGPURenderPassEncoder render_pass);

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
    int window_fb_width;
    int window_fb_height;

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

    // FreeType
    FT_Library FTLibrary;
    R_Font* default_font;

    // memory
    Arena frameArena;

    // render graph
    SG_ID root_pass_id;
    hashmap* frame_uniforms_map; // map from <pipeline_id, camera_id> to bindgroup
    int msaa_sample_count = 4;

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

        int seed                = time(NULL);
        app->frame_uniforms_map = hashmap_new(
          sizeof(FrameUniformsItem), 0, seed, seed, FrameUniformsItem::hash,
          FrameUniformsItem::compare, FrameUniformsItem::free, NULL);
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

        { // Initialize FT and builtin fonts
            FT_Error error = FT_Init_FreeType(&app->FTLibrary);
            if (error) {
                log_fatal("Failed to initialize FreeType\n");
                return;
            }

            R_Font* builtin_font
              = Component_GetFont(&app->gctx, app->FTLibrary, "chugl:cousine-regular");
            ASSERT(builtin_font);

            app->default_font = builtin_font; // safe to store ptr because all fonts are
                                              // kept in static array
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
            // init_info.DepthStencilFormat = app->gctx.depthTextureDesc.format;
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
            CHUGL_Zero_MouseDeltasAndClickState();
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
            // we intentionally are NOT having a fixed timestap for the sake of
            // simplicity.
            // instead, rely on vsync + stable framerate
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
            Material_batchUpdatePipelines(&app->gctx, app->FTLibrary,
                                          app->default_font);
        }

        // process any glfw options passed from chuck
        // UpdateState(scene);

        // garbage collection! delete GPU-side data for any scenegraph objects
        // that were deleted in chuck
        // renderer.ProcessDeletionQueue(
        //   &scene); // IMPORTANT: should happen after flushing command queue

        // now renderer can work on drawing the copied scenegraph
        // renderer.RenderScene(&scene, scene.GetMainCamera());

        // if window minimized, don't render
        if (!GraphicsContext::prepareFrame(&app->gctx)) return;

        // scene
        // TODO RenderPass
        /*
        Refactor render loop
        walk the render graph
        for each RenderPass,
        - create new renderPassDesc, begin and end new render pass
        - call RenderScene with correct params
        - handle window resize
        - handle loadOp [load/clear] and add clearcolor (to scene?)
            - to draw one renderpass on top of another, use loadOp: load
            - need to specify for both color and depth
        - move imgui into it's own non-multisamppled render pass
        - GGen::update() auto update, do on every scene that's in the render
        graph (can test by adding builtin camera controller on chugl side)
        - check window minize still works
        */

        // begin walking render graph
        R_Pass* root_pass = Component_GetPass(app->root_pass_id);
        R_Pass* pass      = Component_GetPass(root_pass->sg_pass.next_pass_id);

        while (pass) {
            switch (pass->sg_pass.pass_type) {
                case SG_PassType_Render: {
                    // defaults to main_scene
                    R_Scene* scene = pass->sg_pass.scene_id != 0 ?
                                       Component_GetScene(pass->sg_pass.scene_id) :
                                       Component_GetScene(app->mainScene);
                    // defaults to scene main camera
                    R_Camera* camera = pass->sg_pass.camera_id != 0 ?
                                         Component_GetCamera(pass->sg_pass.camera_id) :
                                         Component_GetCamera(scene->main_camera_id);
                    // defaults to swapchain current view
                    // TODO: maybe don't need WindowTexture, let null texture default to
                    // window tex? but that would only work in renderpass context...
                    // TODO fix texture creation
                    R_Texture* r_tex
                      = Component_GetTexture(pass->sg_pass.resolve_target_id);

                    // descriptor for view at mip 0
                    WGPUTextureViewDescriptor view_desc = {};
                    view_desc.label                     = "renderpass target view";
                    view_desc.format                    = r_tex->gpu_texture.format;
                    view_desc.dimension                 = WGPUTextureViewDimension_2D;
                    view_desc.baseMipLevel              = 0;
                    view_desc.mipLevelCount             = 1;
                    view_desc.arrayLayerCount           = 1;

                    // no render texture bound, skip this pass
                    if (!r_tex) break;

                    // TODO check auto-rebuild property on RenderPass
                    // assuming always auto-rebuild
                    R_Texture::rebuild(&app->gctx, r_tex, app->window_fb_width,
                                       app->window_fb_height);
                    WGPUTextureView resolve_target_view
                      = wgpuTextureCreateView(r_tex->gpu_texture.texture, &view_desc);
                    WGPUTextureFormat color_attachment_format
                      = r_tex->gpu_texture.format;

                    // it's ok for camera to be null
                    // TODO re-add camera check after adding GCamera default controllers
                    ASSERT(scene && resolve_target_view);

                    {
                        R_Pass::updateRenderPassDesc(
                          &app->gctx, pass, app->window_fb_width, app->window_fb_height,
                          app->msaa_sample_count, resolve_target_view,
                          color_attachment_format);

                        WGPURenderPassEncoder render_pass
                          = wgpuCommandEncoderBeginRenderPass(app->gctx.commandEncoder,
                                                              &pass->render_pass_desc);

                        _R_RenderScene(app, scene, camera, render_pass);

                        wgpuRenderPassEncoderEnd(render_pass);
                        wgpuRenderPassEncoderRelease(render_pass);
                    }

                    // cleanup
                    WGPU_RELEASE_RESOURCE(TextureView, resolve_target_view);
                } break;
                case SG_PassType_Screen: {
                    // TODO support passing texture from chuck
                    WGPUTextureFormat screen_texture_format = app->gctx.swapChainFormat;
                    WGPUTextureView screen_texture_view     = app->gctx.backbufferView;
                    R_Texture* r_tex
                      = Component_GetTexture(pass->sg_pass.screen_texture_id);
                    if (r_tex) {
                        R_Texture::rebuild(&app->gctx, r_tex, app->window_fb_width,
                                           app->window_fb_height);
                        // TODO rebuild
                        screen_texture_view   = r_tex->gpu_texture.view;
                        screen_texture_format = r_tex->gpu_texture.format;
                    }

                    R_Pass::updateScreenPassDesc(&app->gctx, pass, screen_texture_view);
                    WGPURenderPassEncoder render_pass
                      = wgpuCommandEncoderBeginRenderPass(app->gctx.commandEncoder,
                                                          &pass->screen_pass_desc);
                    R_Material* material
                      = Component_GetMaterial(pass->sg_pass.screen_material_id);
                    SG_ID shader_id = material ? material->pso.sg_shader_id : 0;
                    WGPURenderPipeline pipeline = R_GetScreenPassPipeline(
                      &app->gctx, screen_texture_format, shader_id);

                    wgpuRenderPassEncoderSetPipeline(render_pass, pipeline);

                    if (material) {
                        // set bind groups
                        const int screen_pass_binding_location = 0;

                        WGPUBindGroupLayout layout
                          = wgpuRenderPipelineGetBindGroupLayout(
                            pipeline, screen_pass_binding_location);
                        R_Material::rebuildBindGroup(material, &app->gctx, layout);

                        wgpuRenderPassEncoderSetBindGroup(
                          render_pass, screen_pass_binding_location,
                          material->bind_group, 0, NULL);
                    }

                    wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);

                    wgpuRenderPassEncoderEnd(render_pass);
                    wgpuRenderPassEncoderRelease(render_pass);
                } break;
                case SG_PassType_Compute: {
                    R_Shader* compute_shader
                      = Component_GetShader(pass->sg_pass.compute_shader_id);
                    R_Material* compute_material
                      = Component_GetMaterial(pass->sg_pass.compute_material_id);

                    // validation
                    if (compute_material) {
                        ASSERT(compute_material->pso.exclude_from_render_pass);
                        ASSERT(compute_material->pso.sg_shader_id
                               == (compute_shader ? compute_shader->id : 0));
                    }

                    bool valid_compute_pass = compute_material && compute_shader;
                    if (!valid_compute_pass) break;

                    WGPUComputePipeline pipeline
                      = R_GetComputePassPipeline(&app->gctx, compute_shader);

                    WGPUComputePassEncoder compute_pass
                      = wgpuCommandEncoderBeginComputePass(app->gctx.commandEncoder,
                                                           NULL);

                    wgpuComputePassEncoderSetPipeline(compute_pass, pipeline);

                    { // update bind groups
                        const int compute_pass_binding_location = 0;

                        WGPUBindGroupLayout layout
                          = wgpuComputePipelineGetBindGroupLayout(
                            pipeline, compute_pass_binding_location);

                        R_Material::rebuildBindGroup(compute_material, &app->gctx,
                                                     layout);

                        wgpuComputePassEncoderSetBindGroup(
                          compute_pass, compute_pass_binding_location,
                          compute_material->bind_group, 0, NULL);
                    }

                    // dispatch
                    wgpuComputePassEncoderDispatchWorkgroups(
                      compute_pass, pass->sg_pass.workgroup.x,
                      pass->sg_pass.workgroup.y, pass->sg_pass.workgroup.z);

                    // cleanup
                    wgpuComputePassEncoderEnd(compute_pass);
                    WGPU_RELEASE_RESOURCE(ComputePassEncoder, compute_pass);
                }; break;
                case SG_PassType_Bloom: {

                    R_Texture* render_texture = Component_GetTexture(
                      pass->sg_pass.bloom_input_render_texture_id);

                    R_Texture* output_texture = Component_GetTexture(
                      pass->sg_pass.bloom_output_render_texture_id);

                    if (!render_texture || !output_texture) break;

                    // resize output texture
                    // TODO optimize: create all texture views at once
                    R_Texture::rebuild(&app->gctx, output_texture,
                                       render_texture->gpu_texture.width,
                                       render_texture->gpu_texture.height);

                    ASSERT(render_texture->gpu_texture.usage
                           & WGPUTextureUsage_RenderAttachment);

                    SG_Sampler bloom_sampler = {
                        // bilinear, clamp to edge
                        SG_SAMPLER_WRAP_CLAMP_TO_EDGE, SG_SAMPLER_WRAP_CLAMP_TO_EDGE,
                        SG_SAMPLER_WRAP_CLAMP_TO_EDGE, SG_SAMPLER_FILTER_LINEAR,
                        SG_SAMPLER_FILTER_LINEAR,      SG_SAMPLER_FILTER_NEAREST,
                    };
                    u32 bloom_mip_levels
                      = G_mipLevelsLimit(render_texture->gpu_texture.width,
                                         render_texture->gpu_texture.height, 1);
                    bloom_mip_levels
                      = MIN(bloom_mip_levels, pass->sg_pass.bloom_num_blur_levels);
                    ASSERT(bloom_mip_levels
                           <= render_texture->gpu_texture.mip_level_count);

                    if (bloom_mip_levels == 0) break;

                    { // downscale
                        R_Material* bloom_downscale_material = Component_GetMaterial(
                          pass->sg_pass.bloom_downsample_material_id);
                        R_Shader* bloom_downscale_shader = Component_GetShader(
                          bloom_downscale_material->pso.sg_shader_id);
                        ASSERT(bloom_downscale_material->pso.exclude_from_render_pass);
                        WGPUComputePipeline downscale_pipeline
                          = R_GetComputePassPipeline(&app->gctx,
                                                     bloom_downscale_shader);

                        // set the material uniforms that only need to be set once, not
                        // per mip level dispatch
                        R_Material::setSamplerBinding(
                          &app->gctx, bloom_downscale_material, 1, bloom_sampler);

                        ASSERT(render_texture->gpu_texture.mip_level_count > 1);

                        glm::uvec2 mip_size
                          = glm::uvec2(render_texture->gpu_texture.width / 2,
                                       render_texture->gpu_texture.height / 2);
                        ASSERT(sizeof(mip_size) == 2 * sizeof(u32));

                        // downsample, writing from from mip level i --> i + 1
                        for (u32 i = 0; i < bloom_mip_levels - 1; i++) {
                            // create storage texture view for mip level i + 1
                            WGPUTextureViewDescriptor view_desc = {};
                            view_desc.label        = "bloom downscale mip level";
                            view_desc.format       = render_texture->gpu_texture.format;
                            view_desc.dimension    = WGPUTextureViewDimension_2D;
                            view_desc.baseMipLevel = i + 1;
                            view_desc.mipLevelCount   = 1;
                            view_desc.arrayLayerCount = 1;

                            WGPUTextureView dst_mip_view = wgpuTextureCreateView(
                              render_texture->gpu_texture.texture, &view_desc);

                            view_desc.baseMipLevel       = i;
                            WGPUTextureView src_mip_view = wgpuTextureCreateView(
                              render_texture->gpu_texture.texture, &view_desc);

                            // TODO: do we need to refcount the texture view binding?
                            R_Material::setTextureViewBinding(
                              &app->gctx, bloom_downscale_material, 0, src_mip_view);

                            R_Material::setTextureViewBinding(
                              &app->gctx, bloom_downscale_material, 2, dst_mip_view);

                            R_Material::setUniformBinding(
                              &app->gctx, bloom_downscale_material, 3, &i, sizeof(i));

                            const int compute_pass_binding_location = 0;
                            { // update bind group
                                WGPUBindGroupLayout layout
                                  = wgpuComputePipelineGetBindGroupLayout(
                                    downscale_pipeline, compute_pass_binding_location);

                                R_Material::rebuildBindGroup(bloom_downscale_material,
                                                             &app->gctx, layout);
                            }

                            WGPUComputePassEncoder bloom_pass
                              = wgpuCommandEncoderBeginComputePass(
                                app->gctx.commandEncoder, NULL);
                            wgpuComputePassEncoderSetPipeline(bloom_pass,
                                                              downscale_pipeline);

                            wgpuComputePassEncoderSetBindGroup(
                              bloom_pass, compute_pass_binding_location,
                              bloom_downscale_material->bind_group, 0, NULL);

                            // dispatch
                            wgpuComputePassEncoderDispatchWorkgroups(
                              bloom_pass, ceil(mip_size.x / 8.0f),
                              ceil(mip_size.y / 8.0f), 1);

                            mip_size = mip_size / 2u;

                            wgpuComputePassEncoderEnd(bloom_pass);
                            WGPU_RELEASE_RESOURCE(ComputePassEncoder, bloom_pass);

                            // cleanup
                            // cleanup
                            WGPU_RELEASE_RESOURCE(TextureView, src_mip_view);
                            WGPU_RELEASE_RESOURCE(TextureView, dst_mip_view);

                        } // end for
                    } // end downscale

                    { // upscale
                        R_Material* bloom_upscale_material = Component_GetMaterial(
                          pass->sg_pass.bloom_upsample_material_id);
                        R_Shader* bloom_upscale_shader = Component_GetShader(
                          bloom_upscale_material->pso.sg_shader_id);
                        ASSERT(bloom_upscale_material->pso.exclude_from_render_pass);
                        WGPUComputePipeline upscale_pipeline
                          = R_GetComputePassPipeline(&app->gctx, bloom_upscale_shader);
                        // set the material uniforms that only need to be set once, not
                        // per mip level dispatch
                        R_Material::setSamplerBinding(
                          &app->gctx, bloom_upscale_material, 1, bloom_sampler);

                        glm::uvec2 full_res_size
                          = glm::uvec2(render_texture->gpu_texture.width,
                                       render_texture->gpu_texture.height);
                        ASSERT(sizeof(full_res_size) == 2 * sizeof(u32));
                        R_Material::setUniformBinding( // full resolution
                          &app->gctx, bloom_upscale_material, 3, &full_res_size,
                          sizeof(full_res_size));

                        bool first_upsample = true;
                        for (u32 i = bloom_mip_levels - 1; i >= 1; i--) {
                            // calculate size of texture we are writing to
                            G_MipSize mip_size = G_mipLevelSize(
                              render_texture->gpu_texture.width,
                              render_texture->gpu_texture.height, i - 1);

                            // create texture view for src texture
                            WGPUTextureViewDescriptor view_desc = {};
                            view_desc.label        = "bloom upscale mip level";
                            view_desc.format       = render_texture->gpu_texture.format;
                            view_desc.dimension    = WGPUTextureViewDimension_2D;
                            view_desc.baseMipLevel = i;
                            view_desc.mipLevelCount   = 1;
                            view_desc.arrayLayerCount = 1;

                            // TODO don't need to recreate src views, can save from
                            // downsample pass
                            WGPUTextureView src_mip_view = wgpuTextureCreateView(
                              first_upsample ? render_texture->gpu_texture.texture :
                                               output_texture->gpu_texture.texture,
                              &view_desc);

                            view_desc.baseMipLevel       = i - 1;
                            view_desc.mipLevelCount      = 1;
                            WGPUTextureView dst_mip_view = wgpuTextureCreateView(
                              output_texture->gpu_texture.texture, &view_desc);

                            // get view from next next mip in downsample chain
                            WGPUTextureView downsampled_mip_view
                              = wgpuTextureCreateView(
                                render_texture->gpu_texture.texture, &view_desc);

                            // TODO: do we need to refcount the texture view binding?
                            R_Material::setTextureViewBinding(
                              &app->gctx, bloom_upscale_material, 0, src_mip_view);

                            R_Material::setTextureViewBinding(
                              &app->gctx, bloom_upscale_material, 2, dst_mip_view);

                            R_Material::setTextureViewBinding(&app->gctx,
                                                              bloom_upscale_material, 6,
                                                              downsampled_mip_view);

                            // TODO need to have separate uniform buffer for mip levels
                            // overwriting to the same buffer on each dispatch is not
                            // working after queue submission, all mip levels are the
                            // same (i.e. equal 1)
                            // R_Material::setUniformBinding( // mip level
                            //   &app->gctx, bloom_upscale_material, 3, &i, sizeof(i));

                            const int compute_pass_binding_location = 0;
                            { // update bind group
                                WGPUBindGroupLayout layout
                                  = wgpuComputePipelineGetBindGroupLayout(
                                    upscale_pipeline, compute_pass_binding_location);

                                R_Material::rebuildBindGroup(bloom_upscale_material,
                                                             &app->gctx, layout);
                            }

                            WGPUComputePassEncoder bloom_pass
                              = wgpuCommandEncoderBeginComputePass(
                                app->gctx.commandEncoder, NULL);
                            wgpuComputePassEncoderSetPipeline(bloom_pass,
                                                              upscale_pipeline);

                            wgpuComputePassEncoderSetBindGroup(
                              bloom_pass, compute_pass_binding_location,
                              bloom_upscale_material->bind_group, 0, NULL);

                            // dispatch
                            wgpuComputePassEncoderDispatchWorkgroups(
                              bloom_pass, ceil(mip_size.width / 8.0f),
                              ceil(mip_size.height / 8.0f), 1);

                            wgpuComputePassEncoderEnd(bloom_pass);
                            WGPU_RELEASE_RESOURCE(ComputePassEncoder, bloom_pass);

                            // cleanup
                            WGPU_RELEASE_RESOURCE(TextureView, src_mip_view);
                            WGPU_RELEASE_RESOURCE(TextureView, dst_mip_view);
                            WGPU_RELEASE_RESOURCE(TextureView, downsampled_mip_view);
                            first_upsample = false;
                        }
                    }

                    // cleanup

                } break;
                default: ASSERT(false);
            }

            pass = Component_GetPass(pass->sg_pass.next_pass_id);
        }

        // imgui render pass
        if (do_ui) {
            WGPURenderPassColorAttachment imgui_color_attachment = {};
            imgui_color_attachment.view = app->gctx.backbufferView;
#ifdef __EMSCRIPTEN__
            imgui_color_attachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
            imgui_color_attachment.loadOp
              = WGPULoadOp_Load; // DON'T clear the previous frame
            imgui_color_attachment.storeOp = WGPUStoreOp_Store;

            WGPURenderPassDescriptor imgui_render_pass_desc = {};
            imgui_render_pass_desc.label                    = "ImGUI Render Pass";
            imgui_render_pass_desc.colorAttachmentCount     = 1;
            imgui_render_pass_desc.colorAttachments         = &imgui_color_attachment;
            imgui_render_pass_desc.depthStencilAttachment   = NULL;

            WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(
              app->gctx.commandEncoder, &imgui_render_pass_desc);

            ImGui_ImplWGPU_RenderDrawData(&snapshot.DrawData, render_pass);

            wgpuRenderPassEncoderEnd(render_pass);
            wgpuRenderPassEncoderRelease(render_pass);
        }

        GraphicsContext::presentFrame(&app->gctx);

#if 0
// if window not minimized, render
if (GraphicsContext::prepareFrame(&app->gctx)) {

    WGPURenderPassEncoder render_pass = NULL;
    { // render pass
        render_pass = wgpuCommandEncoderBeginRenderPass(
          app->gctx.commandEncoder, &app->gctx.renderPassDesc);

        _R_RenderScene(app, render_pass);

        // UI
        if (do_ui) ImGui_ImplWGPU_RenderDrawData(&snapshot.DrawData, render_pass);

        wgpuRenderPassEncoderEnd(render_pass);
    }

    // submit and present
    GraphicsContext::presentFrame(&app->gctx);

    // cleanup
    // Note: safe to release before submit, immediately after
    // RenderPassEncoderEnd
    wgpuRenderPassEncoderRelease(render_pass);
}
#endif
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

        App* app              = (App*)glfwGetWindowUserPointer(window);
        app->window_fb_width  = width;
        app->window_fb_height = height;

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

        // update chugl state
        if (!app->standalone) {
            switch (button) {
                case GLFW_MOUSE_BUTTON_LEFT:
                    CHUGL_Mouse_LeftButton(action == GLFW_PRESS);
                    break;
                case GLFW_MOUSE_BUTTON_RIGHT:
                    CHUGL_Mouse_RightButton(action == GLFW_PRESS);
                    break;
            }
        }
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

static void _R_RenderScene(App* app, R_Scene* scene, R_Camera* camera,
                           WGPURenderPassEncoder render_pass)
{
    // early out if no pipelines
    if (Component_RenderPipelineCount() == 0) return;

    // Update all transforms
    R_Transform::rebuildMatrices(scene, &app->frameArena);
    // R_Transform::print(main_scene, 0);

    // update camera
    i32 width, height;
    glfwGetWindowSize(app->window, &width, &height);
    f32 aspect = (f32)width / (f32)height;
    if (!camera) {
        // if camera not set by chugl user, use default camera
        app->camera.update(&app->camera, (f32)app->dt);
    }

    // write per-frame uniforms
    f32 time                    = (f32)glfwGetTime();
    FrameUniforms frameUniforms = {};
    frameUniforms.projectionMat = camera ?
                                    R_Camera::projectionMatrix(camera, aspect) :
                                    Camera::projectionMatrix(&app->camera, aspect);
    frameUniforms.viewMat
      = camera ? R_Camera::viewMatrix(camera) : Entity::viewMatrix(&app->camera.entity);
    frameUniforms.projViewMat = frameUniforms.projectionMat * frameUniforms.viewMat;
    frameUniforms.camPos      = app->camera.entity.pos;
    frameUniforms.dirLight    = VEC_FORWARD;
    frameUniforms.time        = time;

    // update frame-level uniforms
    if (camera) {
        bool frame_uniforms_recreated = GPU_Buffer::write(
          &app->gctx, &camera->frame_uniform_buffer, WGPUBufferUsage_Uniform,
          &frameUniforms, sizeof(frameUniforms));
        ASSERT(!frame_uniforms_recreated);
    } else {
        // TODO remove after adding camera controller to chugl, disallowing null camera
        bool frame_uniforms_recreated = GPU_Buffer::write(
          &app->gctx, &R_RenderPipeline::frame_uniform_buffer, WGPUBufferUsage_Uniform,
          &frameUniforms, sizeof(frameUniforms));
        ASSERT(!frame_uniforms_recreated);
    }

    // ==optimize== to prevent sparseness, delete render state entries / arena ids
    // if we find SG_ID arenas are empty
    // - impl only after render loop architecture has stabilized

    // ==optimize== currently iterating over *every* pipeline for each renderpass
    // store a renderpipeline list per R_Scene / renderpass so we don't iterate
    // over pipelines that aren't used in this particular scene
    R_RenderPipeline* render_pipeline = NULL;
    size_t rpIndex                    = 0;
    while (Component_RenderPipelineIter(&rpIndex, &render_pipeline)) {
        ASSERT(render_pipeline->rid != 0);

        if (R_RenderPipeline::numMaterials(render_pipeline) == 0) continue;

        WGPURenderPipeline gpu_pipeline = render_pipeline->gpu_pipeline;

        // ==optimize== cache layouts in R_RenderPipeline struct upon creation
        WGPUBindGroupLayout perMaterialLayout
          = wgpuRenderPipelineGetBindGroupLayout(gpu_pipeline, PER_MATERIAL_GROUP);
        WGPUBindGroupLayout perDrawLayout
          = wgpuRenderPipelineGetBindGroupLayout(gpu_pipeline, PER_DRAW_GROUP);
        WGPUBindGroupLayout vertex_pulling_layout
          = NULL; // set lazily right before draw call

        // set shader
        // ==optimize== only set shader if we actually have anything to render
        wgpuRenderPassEncoderSetPipeline(render_pass, gpu_pipeline);

        if (camera) {
            WGPUBindGroup frame_bind_group;
            FrameUniformsItem key = { render_pipeline->rid, camera->id, NULL };
            FrameUniformsItem* item
              = (FrameUniformsItem*)hashmap_get(app->frame_uniforms_map, &key);

            if (!item) {
                frame_bind_group = FrameUniformsItem::set(
                  app->frame_uniforms_map, &app->gctx, render_pipeline, camera);
                ASSERT(frame_bind_group);
            } else {
                frame_bind_group = item->frame_bindgroup;
            }

            wgpuRenderPassEncoderSetBindGroup(render_pass, PER_FRAME_GROUP,
                                              frame_bind_group, 0, NULL);
        } else {
            // TODO remove after adding chugl camera controller
            wgpuRenderPassEncoderSetBindGroup(render_pass, PER_FRAME_GROUP,
                                              render_pipeline->frame_group, 0, NULL);
        }

        // per-material render loop
        size_t material_idx    = 0;
        R_Material* r_material = NULL;
        while (
          R_RenderPipeline::materialIter(render_pipeline, &material_idx, &r_material)) {

            ASSERT(r_material->pipelineID == render_pipeline->rid);

            MaterialToGeometry* m2g = (MaterialToGeometry*)hashmap_get(
              scene->material_to_geo, &r_material->id);

            // early out scene doesn't use this material
            if (!m2g) continue;
            int geo_count = ARENA_LENGTH(&m2g->geo_ids, SG_ID);
            if (geo_count == 0) continue;

            // set per_material bind group
            // R_Shader* shader = Component_GetShader(r_material->pso.sg_shader_id);
            R_Material::rebuildBindGroup(r_material, &app->gctx, perMaterialLayout);

            wgpuRenderPassEncoderSetBindGroup(render_pass, PER_MATERIAL_GROUP,
                                              r_material->bind_group, 0, NULL);

            // iterate over geometries attached to this material
            for (int geo_idx = 0; geo_idx < geo_count; geo_idx++) {
                R_Geometry* geo = Component_GetGeometry(
                  *ARENA_GET_TYPE(&m2g->geo_ids, SG_ID, geo_idx));
                GeometryToXforms* g2x
                  = R_Scene::getPrimitive(scene, geo->id, r_material->id);
                ASSERT(g2x->key.geo_id == geo->id && g2x->key.mat_id == r_material->id);

                GeometryToXforms::rebuildBindGroup(&app->gctx, scene, g2x,
                                                   perDrawLayout, &app->frameArena);

                // check *after* rebuildBindGroup because some xform ids may be removed
                int num_instances = ARENA_LENGTH(&g2x->xform_ids, SG_ID);
                if (num_instances == 0) continue;

                // set model bind group
                wgpuRenderPassEncoderSetBindGroup(render_pass, PER_DRAW_GROUP,
                                                  g2x->xform_bind_group, 0, NULL);

                // set vertex attributes
                for (int location = 0; location < R_Geometry::vertexAttributeCount(geo);
                     location++) {
                    GPU_Buffer* gpu_buffer = &geo->gpu_vertex_buffers[location];
                    wgpuRenderPassEncoderSetVertexBuffer(
                      render_pass, location, gpu_buffer->buf, 0, gpu_buffer->size);
                }

                // set pulled vertex buffers (programmable vertex pulling)
                if (R_Geometry::usesVertexPulling(geo)) {
                    if (!vertex_pulling_layout) {
                        vertex_pulling_layout = wgpuRenderPipelineGetBindGroupLayout(
                          gpu_pipeline, VERTEX_PULL_GROUP);
                    }
                    R_Geometry::rebuildPullBindGroup(&app->gctx, geo,
                                                     vertex_pulling_layout);
                    wgpuRenderPassEncoderSetBindGroup(render_pass, VERTEX_PULL_GROUP,
                                                      geo->pull_bind_group, 0, NULL);
                }

                // populate index buffer
                u32 num_indices = R_Geometry::indexCount(geo);
                if (num_indices > 0) {
                    // indexed draw
                    wgpuRenderPassEncoderSetIndexBuffer(
                      render_pass, geo->gpu_index_buffer.buf, WGPUIndexFormat_Uint32, 0,
                      geo->gpu_index_buffer.size);

                    wgpuRenderPassEncoderDrawIndexed(render_pass, num_indices,
                                                     num_instances, 0, 0, 0);
                } else {
                    // non-index draw
                    int num_vertices = (int)R_Geometry::vertexCount(geo);
                    int vertex_draw_count
                      = geo->vertex_count >= 0 ? geo->vertex_count : num_vertices;
                    wgpuRenderPassEncoderDraw(render_pass, vertex_draw_count,
                                              num_instances, 0, 0);
                }
            } // foreach geometry
        } // foreach material
    } // foreach pipeline
}

// TODO make sure switch statement is in correct order?
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
                    // doesn't work on macos?
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
        // b2 ----------------------
        case SG_COMMAND_b2_WORLD_SET: {
            SG_Command_b2World_Set* cmd = (SG_Command_b2World_Set*)command;
            app->b2_world_id            = *(b2WorldId*)&cmd->b2_world_id;
        } break;
        case SG_COMMAND_b2_SUBSTEP_COUNT: {
            SG_Command_b2_SubstepCount* cmd = (SG_Command_b2_SubstepCount*)command;
            app->b2_substep_count           = cmd->substep_count;
        } break;
        case SG_COMMAND_GG_SCENE: {
            app->mainScene = ((SG_Command_GG_Scene*)command)->sg_id;
            // update scene's render state
            // TODO check multiple renderpasses, see if this is necessary
            // R_Scene* scene = Component_GetScene(app->mainScene);
            break;
        }
        case SG_COMMAND_CREATE_XFORM:
            Component_CreateTransform((SG_Command_CreateXform*)command);
            break;
        case SG_COMMAND_ADD_CHILD: {
            SG_Command_AddChild* cmd = (SG_Command_AddChild*)command;
            R_Transform::addChild(Component_GetXform(cmd->parent_id),
                                  Component_GetXform(cmd->child_id));
        } break;
        case SG_COMMAND_REMOVE_CHILD: {
            SG_Command_RemoveChild* cmd = (SG_Command_RemoveChild*)command;
            R_Transform::removeChild(Component_GetXform(cmd->parent),
                                     Component_GetXform(cmd->child));
        } break;
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
        } break;
        case SG_COMMAND_SCENE_SET_MAIN_CAMERA: {
            SG_Command_SceneSetMainCamera* cmd
              = (SG_Command_SceneSetMainCamera*)command;
            R_Scene* scene        = Component_GetScene(cmd->scene_id);
            scene->main_camera_id = cmd->camera_id;
        } break;
        // shaders ----------------------
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
            R_Material::setBinding(&app->gctx, material, cmd->location, R_BIND_STORAGE,
                                   data, cmd->data_size_bytes);
        } break;
        case SG_COMMAND_MATERIAL_SET_SAMPLER: {
            SG_Command_MaterialSetSampler* cmd
              = (SG_Command_MaterialSetSampler*)command;
            R_Material* material = Component_GetMaterial(cmd->sg_id);
            R_Material::setSamplerBinding(&app->gctx, material, cmd->location,
                                          cmd->sampler);
        } break;
        case SG_COMMAND_MATERIAL_SET_TEXTURE: {
            SG_Command_MaterialSetTexture* cmd
              = (SG_Command_MaterialSetTexture*)command;
            R_Material* material = Component_GetMaterial(cmd->sg_id);
            R_Material::setTextureBinding(&app->gctx, material, cmd->location,
                                          cmd->texture_id);
        } break;
        case SG_COMMAND_MATERIAL_SET_STORAGE_BUFFER_EXTERNAL: {
            SG_Command_MaterialSetStorageBufferExternal* cmd
              = (SG_Command_MaterialSetStorageBufferExternal*)command;
            R_Material* material = Component_GetMaterial(cmd->material_id);
            R_Buffer* buffer     = Component_GetBuffer(cmd->buffer_id);
            R_Material::setExternalStorageBinding(&app->gctx, material, cmd->location,
                                                  &buffer->gpu_buffer);

        } break;
        // mesh -------------------------
        case SG_COMMAND_MESH_CREATE: {
            SG_Command_Mesh_Create* cmd = (SG_Command_Mesh_Create*)command;
            Component_CreateMesh(cmd);
        } break;
        case SG_COMMAND_CAMERA_CREATE: {
            SG_Command_CameraCreate* cmd = (SG_Command_CameraCreate*)command;
            Component_CreateCamera(&app->gctx, cmd);
        } break;
        case SG_COMMAND_CAMERA_SET_PARAMS: {
            SG_Command_CameraSetParams* cmd = (SG_Command_CameraSetParams*)command;
            R_Camera* camera                = Component_GetCamera(cmd->camera_id);
            camera->params                  = cmd->params;
        } break;
        // text
        case SG_COMMAND_TEXT_REBUILD: {
            SG_Command_TextRebuild* cmd = (SG_Command_TextRebuild*)command;
            Component_CreateText(&app->gctx, app->FTLibrary, cmd);
        } break;
        case SG_COMMAND_TEXT_DEFAULT_FONT: {
            SG_Command_TextDefaultFont* cmd = (SG_Command_TextDefaultFont*)command;
            R_Font* default_font            = Component_GetFont(
              &app->gctx, app->FTLibrary,
              (char*)CQ_ReadCommandGetOffset(cmd->font_path_str_offset));
            if (default_font) app->default_font = default_font;
        } break;
        // pass
        case SG_COMMAND_PASS_CREATE: {
            ASSERT(false);
            SG_Command_PassCreate* cmd = (SG_Command_PassCreate*)command;
            Component_CreatePass(cmd->pass_id);
            if (cmd->pass_type == SG_PassType_Root) {
                app->root_pass_id = cmd->pass_id;
            }
        } break;
        case SG_COMMAND_PASS_UPDATE: {
            SG_Command_PassUpdate* cmd = (SG_Command_PassUpdate*)command;
            R_Pass* pass               = Component_GetPass(cmd->pass.id);

            if (!pass) pass = Component_CreatePass(cmd->pass.id);

            pass->sg_pass = cmd->pass; // copy

            if (cmd->pass.pass_type == SG_PassType_Root) app->root_pass_id = pass->id;
        } break;
        case SG_COMMAND_PASS_CONNECT: {
            // wait, we just reuse PassUpdate for this too
        } break;
        case SG_COMMAND_PASS_DISCONNECT: {
            // wait, we just reuse PassUpdate for this too
        } break;
        // Geometry ---------------------
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
        case SG_COMMAND_GEO_SET_INDICES: {
            SG_Command_GeoSetIndices* cmd = (SG_Command_GeoSetIndices*)command;
            R_Geometry* geo               = Component_GetGeometry(cmd->sg_id);

            u32* indices = (u32*)CQ_ReadCommandGetOffset(cmd->indices_offset);

            R_Geometry::setIndices(&app->gctx, geo, indices, cmd->index_count);
        } break;

        // textures ---------------------
        case SG_COMMAND_TEXTURE_CREATE: {
            SG_Command_TextureCreate* cmd = (SG_Command_TextureCreate*)command;
            Component_CreateTexture(&app->gctx, cmd);
        } break;
        case SG_COMMAND_TEXTURE_DATA: {
            SG_Command_TextureData* cmd = (SG_Command_TextureData*)command;
            R_Texture* texture          = Component_GetTexture(cmd->sg_id);
            void* data                  = CQ_ReadCommandGetOffset(cmd->data_offset);
            R_Texture::write(&app->gctx, texture, data, cmd->width, cmd->height);
        } break;
        case SG_COMMAND_TEXTURE_FROM_FILE: {
            SG_Command_TextureFromFile* cmd = (SG_Command_TextureFromFile*)command;
            R_Texture* texture              = Component_GetTexture(cmd->sg_id);
            const char* path
              = (const char*)CQ_ReadCommandGetOffset(cmd->filepath_offset);
            R_Texture::fromFile(&app->gctx, texture, path);
        } break;
        // buffers ----------------------
        case SG_COMMAND_BUFFER_UPDATE: {
            SG_Command_BufferUpdate* cmd = (SG_Command_BufferUpdate*)command;
            R_Buffer* buffer             = Component_GetBuffer(cmd->buffer_id);
            if (!buffer) {
                buffer = Component_CreateBuffer(cmd->buffer_id);
            }

            // rebuild if necessary
            GPU_Buffer::resizeNoCopy(&app->gctx, &buffer->gpu_buffer, cmd->desc.size,
                                     cmd->desc.usage);
        } break;
        case SG_COMMAND_BUFFER_WRITE: {
            SG_Command_BufferWrite* cmd = (SG_Command_BufferWrite*)command;
            R_Buffer* buffer            = Component_GetBuffer(cmd->buffer_id);
            void* data                  = CQ_ReadCommandGetOffset(cmd->data_offset);

            GPU_Buffer::write(&app->gctx, &buffer->gpu_buffer, buffer->gpu_buffer.usage,
                              cmd->offset_bytes, data, cmd->data_size_bytes);

        } break;
        default: ASSERT(false);
    }
}
#pragma once

#include <stdlib.h>
#include <time.h>

#include <GLFW/glfw3.h>
#include <chuck/chugin.h>
#include <glfw3webgpu/glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include "camera.cpp"
#include "graphics.cpp"
#include "tests/test_base.h"

// forward decls
// struct Chuck_VM;
// struct Chuck_DL_Api;
// typedef const Chuck_DL_Api* CK_DL_API;

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

    // renderer tests
    Camera camera;
    TestCallbacks callbacks;

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

            app->window = glfwCreateWindow(640, 480, "ChuGL", NULL, NULL);

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

        { // set window callbacks
            glfwSetWindowUserPointer(app->window, app);
            glfwSetMouseButtonCallback(app->window, _mouseButtonCallback);
            glfwSetScrollCallback(app->window, _scrollCallback);
            glfwSetCursorPosCallback(app->window, _cursorPositionCallback);
            glfwSetKeyCallback(app->window, _keyCallback);
            glfwSetFramebufferSizeCallback(app->window, _onWindowResize);
        }

        // main loop
#ifdef __EMSCRIPTEN__
        // https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop_arg
        // can't have an infinite loop in emscripten
        // instead pass a callback to emscripten_set_main_loop_arg
        emscripten_set_main_loop_arg(
          [](void* runner) {
              _mainLoop();
              // if (glfwWindowShouldClose(app->window)) {
              //     if (app->callbacks.onExit) app->callbacks.onExit();
              //     emscripten_cancel_main_loop(); // unregister the main loop
              // }
          },
          NULL, // user data (void *)
          -1,   // FPS (negative means use browser's requestAnimationFrame)
          true // simulate infinite loop (prevents code after this from exiting)
        );
#else
        if (app->standalone && app->callbacks.onInit)
            app->callbacks.onInit(&app->gctx, app->window);

        app->camera.entity.pos = glm::vec3(0.0, 0.0, 6.0); // move camera back

        // TODO: probably separate main loops for standalone vs library modes
        log_trace("entering  main loop");
        while (!glfwWindowShouldClose(app->window)) {
            // handle input -------------------
            glfwPollEvents();

            // frame metrics ----------------------------
            ++app->fc;
            _showFPS(app->window);

            if (app->standalone)
                _testLoop(app); // renderer-only tests
            else
                _mainLoop(app); // chuck loop
        }
        log_trace("Exiting main loop");

        if (app->standalone && app->callbacks.onExit) app->callbacks.onExit();
#endif
    }

    static void end(App* app)
    {
        // release graphics context
        GraphicsContext::release(&app->gctx);

        // destroy window
        glfwDestroyWindow(app->window);

        // terminate GLFW
        glfwTerminate();

        // zero all fields
        *app = {};
    }

    // ============================================================================
    // App Internal Functions
    // ============================================================================

    static void _testLoop(App* app)
    {
        // update camera
        // TODO store aspect in app state
        i32 width, height;
        glfwGetWindowSize(app->window, &width, &height);
        f32 aspect = (f32)width / (f32)height;
        app->camera.update(&app->camera, 1.0f / 60.0f); // TODO actually set dt

        if (app->callbacks.onUpdate) app->callbacks.onUpdate(1.0f / 60.0f);
        if (app->callbacks.onRender) {
            app->callbacks.onRender(
              Camera::projectionMatrix(&app->camera, aspect),
              Entity::viewMatrix(&app->camera.entity));
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

        // Render Loop ===========================================
        double prevTickTime = glfwGetTime();

        // ======================
        // enter critical section
        // ======================
        // waiting for audio synchronization (see cgl_update_event_waiting_on)
        // (i.e., when all registered GG.nextFrame() are called on their
        // respective shreds)
        Sync_WaitOnUpdateDone();

        // question: why does putting this AFTER time calculation cause
        // everything to be so choppy at high FPS? hypothesis: puts time
        // calculation into the critical region time is updated only when all
        // chuck shreds are on wait queue guaranteeing that when they awake,
        // they'll be using fresh dt data

        // get current window uptime
        {
            double currentTime = glfwGetTime();
            // time since last frame
            // m_DeltaTime = currentTime - prevTickTime;
            // update for next frame
            // prevTickTime = currentTime;
            UNUSED_VAR(prevTickTime);
            UNUSED_VAR(currentTime);
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

        // CGL::SwapCommandQueues();

        // done swapping the double buffer, let chuck know it's good to continue
        // pushing commands this wakes all shreds currently waiting on
        // GG.nextFrame()
        Event_Broadcast(CHUGL_EventType::NEXT_FRAME, app->ckapi, app->ckvm);

        // ====================
        // end critical section
        // ====================

        // now apply changes from the command queue chuck is NO Longer writing
        // to this executes all commands on the command queue, performs actions
        // from CK code essentially applying a diff to bring graphics state up
        // to date with what is done in CK code
        // CGL::FlushCommandQueue(scene);

        // process any glfw options passed from chuck
        // UpdateState(scene);

        // garbage collection! delete GPU-side data for any scenegraph objects
        // that were deleted in chuck
        // renderer.ProcessDeletionQueue(
        //   &scene); // IMPORTANT: should happen after flushing command queue

        // now renderer can work on drawing the copied scenegraph
        // renderer.RenderScene(&scene, scene.GetMainCamera());

        GraphicsContext::prepareFrame(&app->gctx);
        GraphicsContext::presentFrame(&app->gctx);
    }

    static void _showFPS(GLFWwindow* window)
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

            snprintf(title, WINDOW_TITLE_MAX_LENGTH, "ChuGL-WebGPU [FPS: %.2f]",
                     frameCount / delta);

            log_trace(title);

            glfwSetWindowTitle(window, title);

            frameCount = 0;
            lastTime   = currentTime;
        }
#undef WINDOW_TITLE_MAX_LENGTH
    }

    static void _keyCallback(GLFWwindow* window, int key, int scancode,
                             int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            return;
        }

        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onKey)
            app->callbacks.onKey(key, scancode, action, mods);
    }

    static void _onWindowResize(GLFWwindow* window, int width, int height)
    {
        App* app = (App*)glfwGetWindowUserPointer(window);
        GraphicsContext::resize(&app->gctx, width, height);

        if (app->callbacks.onWindowResize)
            app->callbacks.onWindowResize(width, height);
    }

    static void _mouseButtonCallback(GLFWwindow* window, int button, int action,
                                     int mods)
    {
        log_debug("mouse button callback");
        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onMouseButton)
            app->callbacks.onMouseButton(button, action, mods);

        app->camera.onMouseButton(&app->camera, button, action, mods);
    }

    static void _scrollCallback(GLFWwindow* window, double xoffset,
                                double yoffset)
    {
        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onScroll) app->callbacks.onScroll(xoffset, yoffset);

        app->camera.onScroll(&app->camera, xoffset, yoffset);
    }

    static void _cursorPositionCallback(GLFWwindow* window, double xpos,
                                        double ypos)
    {
        App* app = (App*)glfwGetWindowUserPointer(window);
        if (app->callbacks.onCursorPosition)
            app->callbacks.onCursorPosition(xpos, ypos);

        app->camera.onCursorPosition(&app->camera, xpos, ypos);
    }
};

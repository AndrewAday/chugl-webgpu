#include "app.h"
#include "sync.h"

#include "core/log.h"
#include "core/macros.h"

#include <stdlib.h>
#include <time.h>

#include <GLFW/glfw3.h>
#include <glfw3webgpu/glfw3webgpu.h>

// ============================================================================
// Globals
// ============================================================================
CHUGL_App app = {};

static void showFPS(GLFWwindow* window)
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

        snprintf(title, WINDOW_TITLE_MAX_LENGTH, "WebGPU-Renderer [FPS: %.2f]",
                 frameCount / delta);

        log_trace(title);

        glfwSetWindowTitle(window, title);

        frameCount = 0;
        lastTime   = currentTime;
    }

#undef WINDOW_TITLE_MAX_LENGTH
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action,
                        int mods)
{
    UNUSED_VAR(scancode);
    UNUSED_VAR(mods);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
}

static void mainLoop()
{
    glfwPollEvents();
    showFPS(app.window);

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
    Event_Broadcast(CHUGL_EventType::NEXT_FRAME, app.ckapi, app.ckvm);

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

    GraphicsContext::prepareFrame(&app.gctx);
    GraphicsContext::presentFrame(&app.gctx);
}

// ============================================================================
// Definitions
// ============================================================================
void CHUGL_App::init(Chuck_VM* vm, CK_DL_API api)
{
    ASSERT(app.ckvm == NULL && app.ckapi == NULL);
    app.ckvm  = vm;
    app.ckapi = api;
}

void CHUGL_App::start()
{
    ASSERT(app.window == NULL);

    // seed random number generator ===========================
    srand((unsigned int)time(0));

    { // Initialize window
        if (!glfwInit()) {
            log_fatal("Failed to initialize GLFW\n");
            return;
        }

        // Create the window without an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        app.window = glfwCreateWindow(640, 480, "ChuGL", NULL, NULL);

        // TODO: set window user pointer to CHUGL_App

        if (!app.window) {
            log_fatal("Failed to create GLFW window\n");
            glfwTerminate();
            return;
        }
    }

    // init graphics context
    if (!GraphicsContext::init(&app.gctx, app.window)) {
        log_fatal("Failed to initialize graphics context\n");
        return;
    }

    { // set window callbacks (TODO)
        glfwSetKeyCallback(app.window, keyCallback);
    }

    // main loop
#ifdef __EMSCRIPTEN__
    // https://emscripten.org/docs/api_reference/emscripten.h.html#c.emscripten_set_main_loop_arg
    // can't have an infinite loop in emscripten
    // instead pass a callback to emscripten_set_main_loop_arg
    emscripten_set_main_loop_arg(
      [](void* runner) {
          mainLoop();
          // if (glfwWindowShouldClose(runner->window)) {
          //     if (runner->callbacks.onExit) runner->callbacks.onExit();
          //     emscripten_cancel_main_loop(); // unregister the main loop
          // }
      },
      NULL, // user data (void *)
      -1,   // FPS (negative means use browser's requestAnimationFrame)
      true  // simulate infinite loop (prevents code after this from exiting)
    );
#else
    while (!glfwWindowShouldClose(app.window)) mainLoop();
#endif
}

void CHUGL_App::end()
{
    // release graphics context
    GraphicsContext::release(&app.gctx);

    // destroy window
    glfwDestroyWindow(app.window);
    app.window = NULL;

    // terminate GLFW
    glfwTerminate();
}
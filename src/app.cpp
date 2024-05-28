#pragma once

#include <stdlib.h>
#include <time.h>

#include <GLFW/glfw3.h>
#include <chuck/chugin.h>
#include <glfw3webgpu/glfw3webgpu.h>
#include <glm/gtx/string_cast.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include "camera.cpp"
#include "graphics.h"
#include "r_component.h"
#include "sg_command.h"
#include "sg_component.h"
#include "tests/test_base.h"

// forward decls
// struct Chuck_VM;
// struct Chuck_DL_Api;
// typedef const Chuck_DL_Api* CK_DL_API;

// command handling table

// typedef void (*R_CommandFunc)(App* app, SG_Command* command);

// struct R_CommandHandler {
//     SG_CommandType type;

// }

// {
//     SG_CommandType->fn(App, SG_CommandSubclass)
// }

static void _R_HandleCommand(SG_Command* command)
{
    switch (command->type) {
        case SG_COMMAND_TYPE_CREATE_XFORM:
            Component_CreateTransform((SG_Command_CreateXform*)command);
            break;
        case SG_COMMAND_TYPE_ADD_CHILD: {
            SG_Command_AddChild* cmd = (SG_Command_AddChild*)command;
            R_Transform::addChild(Component_GetXform(cmd->parent_id),
                                  Component_GetXform(cmd->child_id));
            break;
        }
        case SG_COMMAND_TYPE_REMOVE_CHILD: {
            SG_Command_RemoveChild* cmd = (SG_Command_RemoveChild*)command;
            R_Transform::removeChild(Component_GetXform(cmd->parent),
                                     Component_GetXform(cmd->child));
            break;
        }
        case SG_COMMAND_TYPE_SET_POSITION: {
            SG_Command_SetPosition* cmd = (SG_Command_SetPosition*)command;
            R_Transform::pos(Component_GetXform(cmd->sg_id), cmd->pos);
            break;
        }
        case SG_COMMAND_TYPE_SET_ROTATATION: {
            SG_Command_SetRotation* cmd = (SG_Command_SetRotation*)command;
            R_Transform::rot(Component_GetXform(cmd->sg_id), cmd->rot);
            break;
        }
        case SG_COMMAND_TYPE_SET_SCALE: {
            SG_Command_SetScale* cmd = (SG_Command_SetScale*)command;
            R_Transform::sca(Component_GetXform(cmd->sg_id), cmd->sca);
            break;
        }
        default: ASSERT(false);
    }
}

static void _R_RenderScene(App* app)
{
    // Update all transforms
    R_Transform::rebuildMatrices(Component_GetXform(sceneIDs[0]), &frameArena);

    R_Transform::print(Component_GetXform(sceneIDs[0]), 0);

    WGPURenderPassEncoder renderPass = GraphicsContext::prepareFrame(gctx);

    // write per-frame uniforms
    f32 time                    = (f32)glfwGetTime();
    FrameUniforms frameUniforms = {};
    frameUniforms.projectionMat = proj;
    frameUniforms.viewMat       = view;
    frameUniforms.projViewMat
      = frameUniforms.projectionMat * frameUniforms.viewMat;
    frameUniforms.camPos   = camPos;
    frameUniforms.dirLight = VEC_FORWARD;
    frameUniforms.time     = time;

    // log_debug("geo num instances: %d", R_Geometry::numInstances(geo));
    // Test render loop
    // use bool Component_RenderPipelineIter(size_t* i, R_RenderPipeline**
    // renderPipeline);
    // TODO move into renderer.cpp
    R_RenderPipeline* renderPipeline = NULL;
    size_t rpIndex                   = 0;
    while (Component_RenderPipelineIter(&rpIndex, &renderPipeline)) {
        // log_trace("drawing materials for render pipeline: %d",
        //           renderPipeline->rid);

        // TODO: early out if numMaterials == 0;

        WGPURenderPipeline gpuPipeline = renderPipeline->pipeline.pipeline;
        // TODO: cache the bindGroupLayout in the pipeline after creation (it
        // will never change)
        WGPUBindGroupLayout perMaterialLayout
          = wgpuRenderPipelineGetBindGroupLayout(gpuPipeline,
                                                 PER_MATERIAL_GROUP);
        WGPUBindGroupLayout perDrawLayout
          = wgpuRenderPipelineGetBindGroupLayout(gpuPipeline, PER_DRAW_GROUP);

        // set shader
        wgpuRenderPassEncoderSetPipeline(renderPass, gpuPipeline);

        // set frame bind group (needs to be set per renderpipeline as long as
        // we use implicit layout:auto)
        wgpuQueueWriteBuffer(gctx->queue,
                             renderPipeline->pipeline.frameUniformBuffer, 0,
                             &frameUniforms, sizeof(frameUniforms));
        wgpuRenderPassEncoderSetBindGroup(renderPass, PER_FRAME_GROUP,
                                          renderPipeline->pipeline.frameGroup,
                                          0, NULL);

        // per-material render loop
        size_t materialIdx    = 0;
        R_Material* rMaterial = NULL;

        while (R_RenderPipeline::materialIter(renderPipeline, &materialIdx,
                                              &rMaterial)) {
            // get material
            // log_trace("drawing material: %d", rMaterial->id);

            ASSERT(rMaterial && rMaterial->pipelineID == renderPipeline->rid);

            // TODO: early out on rMaterial->numPrimitives == 0

            // TODO: figure out textures / texture views...

            // set per_material bind group
            R_Material::rebuildBindGroup(rMaterial, gctx, perMaterialLayout);
            wgpuRenderPassEncoderSetBindGroup(renderPass, PER_MATERIAL_GROUP,
                                              rMaterial->bindGroup, 0, NULL);

            // iterate over material primitives
            size_t primitiveIdx           = 0;
            Material_Primitive* primitive = NULL;
            while (
              R_Material::primitiveIter(rMaterial, &primitiveIdx, &primitive)) {
                u32 numInstances = Material_Primitive::numInstances(primitive);
                if (numInstances == 0) continue;
                Material_Primitive::rebuildBindGroup(
                  gctx, primitive, perDrawLayout, &frameArena);

                // set model bind group
                wgpuRenderPassEncoderSetBindGroup(
                  renderPass, PER_DRAW_GROUP, primitive->bindGroup, 0, NULL);

                R_Geometry* geo = Component_GetGeo(primitive->geoID);
                ASSERT(geo);

                // set vertex attributes
                // TODO: move into renderer, and consider separate buffer for
                // each attribute to handle case where some vertex attributes
                // are missing (tangent, color, etc)
                // TODO: what happens if a vertex attribute is not set in for
                // the shader?
                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 0, geo->gpuVertexBuffer, 0,
                  sizeof(f32) * geo->numVertices * 3);

                auto normalsOffset = sizeof(f32) * geo->numVertices * 3;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 1, geo->gpuVertexBuffer, normalsOffset,
                  sizeof(f32) * geo->numVertices * 3);

                auto texcoordsOffset = sizeof(f32) * geo->numVertices * 6;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 2, geo->gpuVertexBuffer, texcoordsOffset,
                  sizeof(f32) * geo->numVertices * 2);

                size_t tangentOffset = sizeof(f32) * geo->numVertices * 8;

                wgpuRenderPassEncoderSetVertexBuffer(
                  renderPass, 3, geo->gpuVertexBuffer, tangentOffset,
                  sizeof(f32) * geo->numVertices * 4);

                // populate index buffer
                if (geo->numIndices > 0)
                    wgpuRenderPassEncoderSetIndexBuffer(
                      renderPass, geo->gpuIndexBuffer, WGPUIndexFormat_Uint32,
                      0, geo->indexBufferDesc.size);

                // draw call (indexed)
                if (geo->numIndices > 0) {
                    wgpuRenderPassEncoderDrawIndexed(
                      renderPass, geo->numIndices, numInstances, 0, 0, 0);
                } else {
                    // draw call (nonindexed)
                    wgpuRenderPassEncoderDraw(renderPass, geo->numVertices,
                                              numInstances, 0, 0);
                }
            }

            // material uniforms TODO switch to pbr
            // MaterialUniforms materialUniforms = {};
            // materialUniforms.color            =
            // glm::vec4(1.0, 1.0, 1.0, 1.0);
            // // TODO: only need to write if it's stale/changed
            // wgpuQueueWriteBuffer(gctx->queue, //
            //                      material.uniformBuffer, // 0, //
            //                      &materialUniforms, sizeof(materialUniforms)
            //                      //
            // );
        }
    }

    GraphicsContext::presentFrame(gctx);

    // end of frame, clear arena
    Arena::clear(&frameArena);
}

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

        // initialize R_Component manager
        Component_Init();
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
            {
                _showFPS(app->window);

                ++app->fc;
                f64 currentTime = glfwGetTime();

                // first frame prevent huge dt
                if (app->lastTime == 0) app->lastTime = currentTime;

                app->dt       = currentTime - app->lastTime;
                app->lastTime = currentTime;
            }

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
        // free R_Components
        Component_Free();

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

        if (app->callbacks.onUpdate) app->callbacks.onUpdate(app->dt);
        if (app->callbacks.onRender) {
            app->callbacks.onRender(
              Camera::projectionMatrix(&app->camera, aspect),
              Entity::viewMatrix(&app->camera.entity), app->camera.entity.pos);
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

        Sync_SwapCommandQueues();

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

        // CGL::FlushCommandQueue(scene); // TODO next!
        { // flush command queue
            SG_Command* cmd = NULL;
            while (Sync_ReadCommandQueueIter(&cmd)) _R_HandleCommand(cmd);
            Sync_ReadCommandQueueClear();
        }

        // process any glfw options passed from chuck
        // UpdateState(scene);

        // garbage collection! delete GPU-side data for any scenegraph objects
        // that were deleted in chuck
        // renderer.ProcessDeletionQueue(
        //   &scene); // IMPORTANT: should happen after flushing command queue

        // now renderer can work on drawing the copied scenegraph
        // renderer.RenderScene(&scene, scene.GetMainCamera());
        _R_RenderScene();

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

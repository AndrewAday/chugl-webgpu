// clang-format off
#include "all.cpp"
#include "ulib_helper.h"

// ulibs
#include "ulib_box2d.cpp"
#include "ulib_component.cpp"
#include "ulib_camera.cpp"
#include "ulib_scene.cpp"
#include "ulib_geometry.cpp"
#include "ulib_imgui.cpp"
#include "ulib_material.cpp"
#include "ulib_texture.cpp"
#include "ulib_window.cpp"

// vendor
#include <sokol/sokol_time.h>
// clang-format on

// ChuGL version string
#define CHUGL_VERSION_STRING "0.1.5 (alpha)"

static Chuck_DL_MainThreadHook* hook = NULL;
static bool hookActivated            = false;

// metadata required for scene rendering
struct GG_Config {
    SG_ID mainScene;
    SG_ID mainCamera;
};

GG_Config gg_config = {};

static f64 ckdt_sec      = 0;
static f64 system_dt_sec = 0;

t_CKBOOL chugl_main_loop_hook(void* bindle)
{
    UNUSED_VAR(bindle);
    log_trace("chugl_main_loop_hook");

    ASSERT(g_chuglAPI && g_chuglVM);

    App app = {};

    App::init(&app, g_chuglVM, g_chuglAPI);
    App::start(&app); // blocking

    { // cleanup (after exiting main loop)
        // remove all shreds (should trigger shutdown, unless running in --loop
        // mode)
        if (g_chuglVM && g_chuglAPI) g_chuglAPI->vm->remove_all_shreds(g_chuglVM);

        App::end(&app);
    }

    return true;
}

t_CKBOOL chugl_main_loop_quit(void* bindle)
{
    UNUSED_VAR(bindle);
    SG_Free();
    CQ_Free();
    return true;
}

// ChuGL chugin info func
CK_DLL_INFO(ChuGL)
{
    // set module name
    QUERY->setname(QUERY, "ChuGL");

    // set info
    QUERY->setinfo(QUERY, CHUGIN_INFO_AUTHORS, "Andrew Zhu Aday & Ge Wang");
    QUERY->setinfo(QUERY, CHUGIN_INFO_CHUGIN_VERSION, CHUGL_VERSION_STRING);
    QUERY->setinfo(
      QUERY, CHUGIN_INFO_DESCRIPTION,
      "ChuGL (ChucK Graphics Library) is a unified audiovisual programming "
      "framework built into the ChucK programming language.");
    QUERY->setinfo(QUERY, CHUGIN_INFO_URL, "https://chuck.stanford.edu/chugl/");
    QUERY->setinfo(QUERY, CHUGIN_INFO_EMAIL, "azaday@ccrma.stanford.edu");

    { // setup
        // initialize performance counters
        stm_setup();
    }
}

// ============================================================================
// Query
// ============================================================================

static t_CKUINT chugl_next_frame_event_data_offset = 0;

static void autoUpdateScenegraph(Arena* arena, SG_ID main_scene_id, Chuck_VM* VM,
                                 CK_DL_API API, t_CKINT _ggen_update_vt_offset)
{
    static t_CKTIME chuglLastUpdateTime{};

    t_CKTIME chuckTimeNow  = API->vm->now(VM);
    t_CKTIME chuckTimeDiff = chuckTimeNow - chuglLastUpdateTime;
    t_CKFLOAT ckdt         = chuckTimeDiff / API->vm->srate(VM);
    chuglLastUpdateTime    = chuckTimeNow;

    Chuck_DL_Arg theArg;
    theArg.kind = kindof_FLOAT;
    // if (CGL::useChuckTime) {
    theArg.value.v_float = ckdt;
    // } else {
    // TODO: pass window state, dt, etc from render thread --> audio thread
    // theArg.value.v_float = std::min(
    //   1.0, CGL::GetTimeInfo().second); // this dt should be same as one
    //                                    // gotten by chuck CGL.dt()
    // }

    void* arena_orig_top             = Arena::top(arena);
    *(ARENA_PUSH_TYPE(arena, SG_ID)) = main_scene_id;

    // BFS through graph
    // TODO: can just walk linearly through entity arenas instead?
    // but then how do we know if something is part of the active scene graph?
    while (Arena::top(arena) != arena_orig_top) {
        ARENA_POP_TYPE(arena, SG_ID);
        SG_ID sg_id = *(SG_ID*)Arena::top(arena);

        SG_Transform* xform = SG_GetTransform(sg_id);
        ASSERT(xform != NULL);

        Chuck_Object* ggen = xform->ckobj;
        ASSERT(ggen != NULL);

        Chuck_VM_Shred* origin_shred = chugin_getOriginShred(ggen);

        // origin_shred == NULL when ggens are default created during QUERY
        // API->create_without_shred e.g. for mainScene, mainCamera
        // these, by definition, have no overloaded update() function
        // because they are not user defined
        if (origin_shred != NULL) {
            // invoke the update function in immediate mode
            API->vm->invoke_mfun_immediate_mode(ggen, _ggen_update_vt_offset, VM,
                                                origin_shred, &theArg, 1);
        }

        // add children to stack
        size_t numChildren  = SG_Transform::numChildren(xform);
        SG_ID* children_ptr = ARENA_PUSH_COUNT(arena, SG_ID, numChildren);
        memcpy(children_ptr, xform->childrenIDs.base, sizeof(SG_ID) * numChildren);
    }
}

CK_DLL_SFUN(chugl_next_frame)
{
    // extract CglEvent from obj
    // TODO: workaround bug where create() object API is not calling
    // preconstructors
    // https://trello.com/c/JwhVQEpv/48-cglnextframe-now-not-calling-preconstructor-of-cglupdate

    if (!Sync_HasShredWaited(SHRED))
        API->vm->throw_exception(
          "NextFrameNotWaitedOnViolation",
          "You are calling .nextFrame() without chucking to now!\n"
          "Please replace this line with .nextFrame() => now;",
          SHRED);

    // RETURN->v_object = (Chuck_Object *)CGL::GetShredUpdateEvent(SHRED, API,
    // VM)->GetEvent();
    RETURN->v_object = (Chuck_Object*)Event_Get(CHUGL_EventType::NEXT_FRAME, API, VM);

    // register shred and set has-waited flag to false
    Sync_RegisterShred(SHRED);

    // bugfix: grabbing this lock prevents race with render thread
    // broadcasting nextFrameEvent and setting waitingShreds to 0.
    // Unlocked in event_next_frame_waiting_on after shred has
    // been added to the nextFrameEvent waiting queue.
    // Render thread holds this lock when broadcasting + setting waitingShreds
    // to 0
    spinlock::lock(&waitingShredsLock);
}

//-----------------------------------------------------------------------------
// this is called by chuck VM at the earliest point when a shred begins to wait
// on an Event used to catch GG.nextFrame() => now; on one or more shreds once
// all expected shreds are waiting on GG.nextFrame(), this function signals the
// graphics-side
//-----------------------------------------------------------------------------
CK_DLL_MFUN(event_next_frame_waiting_on)
{
    // see comment in chugl_next_frame
    ++waitingShreds;
    bool allShredsWaiting = waitingShreds == registeredShreds.size();
    spinlock::unlock(&waitingShredsLock);

    ASSERT(registeredShreds.find(SHRED) != registeredShreds.end());
    registeredShreds[SHRED] = true; // mark shred waited

    // THIS IS A VERY IMPORTANT FUNCTION. See
    // https://trello.com/c/Gddnu21j/6-chuglrender-refactor-possible-deadlock-between-cglupdate-and-render
    // and
    // https://github.com/ccrma/chugl/blob/2023-chugl-int/design/multishred-render-1.ck
    // for further context

    // activate hook only on GG.nextFrame();
    if (!hookActivated) {
        hookActivated = true;
        hook->activate(hook);
    }

    if (allShredsWaiting) {
        // if #waiting == #registered, all chugl shreds have finished work, and
        // we are safe to wakeup the renderer
        // TODO: bug. If a shred does NOT call GG.nextFrame in an infinite loop,
        // i.e. does nextFrame() once and then goes on to say process audio,
        // this code will stay be expecting that shred to call nextFrame() again
        // and waitingShreds will never == registeredShreds.size() thus hanging
        // the renderer

        { // process dt
            // process dt in audio samples
            static t_CKTIME chuglLastUpdateTime{ API->vm->now(VM) };

            t_CKTIME chuckTimeNow  = API->vm->now(VM);
            t_CKTIME chuckTimeDiff = chuckTimeNow - chuglLastUpdateTime;
            ckdt_sec               = chuckTimeDiff / API->vm->srate(VM);
            chuglLastUpdateTime    = chuckTimeNow;

            // process dt with OS-provided timer
            static u64 system_last_time{ stm_now() };
            u64 system_dt_ticks = stm_laptime(&system_last_time);
            system_dt_sec       = stm_sec(system_dt_ticks);
        }

        // traverse scenegraph and call chuck-defined update() on all GGens
        autoUpdateScenegraph(&audio_frame_arena, gg_config.mainScene, g_chuglVM,
                             g_chuglAPI, ggen_update_vt_offset);

        // signal the graphics-side that audio-side is done processing for
        // this frame
        Sync_SignalUpdateDone();

        // Garbage collect (TODO add API function to control this via GG
        // config) SG_GC();

        // clear audio frame arena
        Arena::clear(&audio_frame_arena);
    }
}

CK_DLL_SFUN(chugl_gc)
{
    SG_GC();
}

CK_DLL_SFUN(chugl_get_scene)
{
    RETURN->v_object = SG_GetScene(gg_config.mainScene)->ckobj;
}

CK_DLL_SFUN(chugl_set_scene)
{
    SG_ID prev_scene_id = gg_config.mainScene;

    // get new scene
    Chuck_Object* newScene = GET_NEXT_OBJECT(ARGS);
    SG_Scene* sg_scene = SG_GetScene(OBJ_MEMBER_UINT(newScene, component_offset_id));

    // bump refcount on new scene
    SG_AddRef(sg_scene);

    // assign new scene
    gg_config.mainScene = sg_scene ? sg_scene->id : 0;
    CQ_PushCommand_GG_Scene(sg_scene);

    // decrement refcount on old scene
    SG_DecrementRef(prev_scene_id);
}

CK_DLL_SFUN(chugl_get_fps)
{
    RETURN->v_float = CHUGL_Window_fps();
    return;
}

CK_DLL_SFUN(chugl_get_dt)
{
    RETURN->v_float = CHUGL_Window_dt();
}

// ============================================================================
// Chugin entry point
// ============================================================================
CK_DLL_QUERY(ChuGL)
{

    // remember
    g_chuglVM  = QUERY->ck_vm(QUERY);
    g_chuglAPI = QUERY->ck_api(QUERY);

    // audio frame arena
    Arena::init(&audio_frame_arena, 64 * KILOBYTE);

    // initialize component pool
    // TODO: have a single ChuGL_Context that manages this all
    SG_Init(g_chuglAPI);
    CQ_Init();

    // set up for main thread hook, for running ChuGL on the main thread
    hook = QUERY->create_main_thread_hook(QUERY, chugl_main_loop_hook,
                                          chugl_main_loop_quit, NULL);

    // TODO: add shred_on_destroy listener

    // initialize ChuGL API ========================================
    { // chugl events
        // triggered by main render thread after deepcopy is complete, and safe
        // for chuck to begin updating the scene graph (intentionally left out
        // of CKDocs)
        QUERY->begin_class(QUERY, "NextFrameEvent", "Event");
        DOC_CLASS(
          "Don't instantiate this class directly. Use GG.nextFrame() => now; "
          "instead.");
        // no destructor for singleton
        chugl_next_frame_event_data_offset
          = QUERY->add_mvar(QUERY, "int", "@next_frame_event_data", false);

        QUERY->add_mfun(QUERY, event_next_frame_waiting_on, "void", "waiting_on");
        QUERY->end_class(QUERY);
    }

    // auto-generated
    // ulib_cimgui_query(QUERY);
    // hand-written
    ulib_imgui_query(QUERY);
    ulib_box2d_query(QUERY);

    ulib_window_query(QUERY);
    ulib_component_query(QUERY);
    ulib_texture_query(QUERY);
    ulib_ggen_query(QUERY);
    ulib_camera_query(QUERY);
    ulib_gscene_query(QUERY);
    ulib_geometry_query(QUERY);
    ulib_material_query(QUERY);
    ulib_mesh_query(QUERY);

    static u64 foo = 12345;
    { // GG static functions
        QUERY->begin_class(QUERY, "GG", "Object");

        QUERY->add_svar(QUERY, "int", "foo", true, &foo);

        QUERY->add_sfun(QUERY, chugl_next_frame, "NextFrameEvent", "nextFrame");
        QUERY->doc_func(
          QUERY,
          "Registers the calling shred to be notified when the next frame is "
          "finished rendering. When all graphics shreds are finished calling."
          "Note: this function returns an event that MUST be waited on for "
          "correct behavior, i.e. GG.nextFrame() => now;"
          "See the ChuGL tutorial and examples for more information.");

        QUERY->add_sfun(QUERY, chugl_get_scene, SG_CKNames[SG_COMPONENT_SCENE],
                        "scene");
        QUERY->add_sfun(QUERY, chugl_set_scene, SG_CKNames[SG_COMPONENT_SCENE],
                        "scene");
        QUERY->add_arg(QUERY, SG_CKNames[SG_COMPONENT_SCENE], "scene");

        // QUERY->add_sfun(QUERY, chugl_gc, "void", "gc");
        // QUERY->doc_func(QUERY, "Trigger garbage collection");

        SFUN(chugl_get_fps, "float", "fps");
        DOC_FUNC("FPS of current window, updated every second");

        // note: we use window time here instead of ckdt_sec or system_dt_sec
        // (which are laptimes calculated on the audio thread every cycle of
        // nextFrameWaitingOn ) because the latter values are highly unstable, and do
        // not correspond to the actual average FPS of the graphics window. dt is most
        // likely used for graphical animations, and so therefore should be set by the
        // actual render thread, not the audio thread.
        SFUN(chugl_get_dt, "float", "dt");
        DOC_FUNC("return the laptime of the graphics thread's last frame in seconds");

        QUERY->end_class(QUERY); // GG
    }

    { // Default components
        // scene
        Chuck_DL_Api::Type sceneCKType
          = g_chuglAPI->type->lookup(g_chuglVM, SG_CKNames[SG_COMPONENT_SCENE]);
        Chuck_DL_Api::Object sceneObj
          = g_chuglAPI->object->create_without_shred(g_chuglVM, sceneCKType, true);
        SG_Scene* scene
          = CQ_PushCommand_SceneCreate(sceneObj, component_offset_id, g_chuglAPI);
        gg_config.mainScene = scene->id;
        CQ_PushCommand_GG_Scene(scene);
    }

    // wasn't that a breeze?
    return true;
}

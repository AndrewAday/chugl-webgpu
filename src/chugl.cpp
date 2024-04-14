#include <chuck/chugin.h>

#include "app.h"
#include "core/log.h"
#include "core/macros.h"

#include "sync.h"

// ChuGL version string
#define CHUGL_VERSION_STRING "0.1.5 (alpha)"

// references to VM and API
static Chuck_VM* g_chuglVM  = NULL;
static CK_DL_API g_chuglAPI = NULL;

static Chuck_DL_MainThreadHook* hook = NULL;
static bool hookActivated            = false;

t_CKBOOL chugl_main_loop_hook(void* bindle)
{
    UNUSED_VAR(bindle);
    log_trace("chugl_main_loop_hook");

    ASSERT(g_chuglAPI && g_chuglVM);
    CHUGL_App::init(g_chuglVM, g_chuglAPI);
    CHUGL_App::start();

    { // cleanup (after exiting main loop)
        // remove all shreds (should trigger shutdown, unless running in --loop
        // mode)
        if (g_chuglVM && g_chuglAPI)
            g_chuglAPI->vm->remove_all_shreds(g_chuglVM);

        CHUGL_App::end();
    }

    return TRUE;
}

t_CKBOOL chugl_main_loop_quit(void* bindle)
{
    UNUSED_VAR(bindle);
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
}

// ============================================================================
// Query
// ============================================================================

static t_CKUINT chugl_next_frame_event_data_offset = 0;

/// @brief NextFrameEvent singleton
CK_DLL_CTOR(event_next_frame_ctor)
{
    // store reference to our new class
    OBJ_MEMBER_INT(SELF, chugl_next_frame_event_data_offset)
      = (t_CKINT)Event_Get(CHUGL_EventType::NEXT_FRAME, API, VM);
}

//-----------------------------------------------------------------------------
// this is called by chuck VM at the earliest point when a shred begins to wait
// on an Event used to catch GG.nextFrame() => now; on one or more shreds once
// all expected shreds are waiting on GG.nextFrame(), this function signals the
// graphics-side
//-----------------------------------------------------------------------------
CK_DLL_MFUN(event_next_frame_waiting_on)
{
    // THIS IS A VERY IMPORTANT FUNCTION. See
    // https://trello.com/c/Gddnu21j/6-chuglrender-refactor-possible-deadlock-between-cglupdate-and-render
    // and
    // https://github.com/ccrma/chugl/blob/2023-chugl-int/design/multishred-render-1.ck
    // for further context

    // activate hook only on GG.nextFrame();
    if (!hookActivated) {
        log_trace("event_next_frame_waiting_on: activating hook");
        hook->activate(hook);
        hookActivated = true;
    }

    Sync_MarkShredWaited(SHRED);

    log_trace("event_next_frame_waiting_on");
}

CK_DLL_SFUN(chugl_next_frame)
{
    // extract CglEvent from obj
    // TODO: workaround bug where create() object API is not calling
    // preconstructors
    // https://trello.com/c/JwhVQEpv/48-cglnextframe-now-not-calling-preconstructor-of-cglupdate

    // initialize ckobj for mainScene
    // CGL::GetMainScene(SHRED, API, VM);
    // TODO: init should happen at start of main hook

    if (!Sync_HasShredWaited(SHRED))
        API->vm->throw_exception(
          "NextFrameNotWaitedOnViolation",
          "You are calling .nextFrame() without chucking to now!\n"
          "Please replace this line with .nextFrame() => now;",
          SHRED);

    // RETURN->v_object = (Chuck_Object *)CGL::GetShredUpdateEvent(SHRED, API,
    // VM)->GetEvent();
    RETURN->v_object
      = (Chuck_Object*)Event_Get(CHUGL_EventType::NEXT_FRAME, API, VM);

    // register shred and set has-waited flag to false
    Sync_RegisterShred(SHRED);
}

// ============================================================================
// Chugin entry point
// ============================================================================
CK_DLL_QUERY(ChuGL)
{
    // set up for main thread hook, for running ChuGL on the main thread
    hook = QUERY->create_main_thread_hook(QUERY, chugl_main_loop_hook,
                                          chugl_main_loop_quit, NULL);

    // initialize ChuGL API ========================================
    { // chugl events
        // triggered by main render thread after deepcopy is complete, and safe
        // for chuck to begin updating the scene graph (intentionally left out
        // of CKDocs)
        QUERY->begin_class(QUERY, "NextFrameEvent", "Event");
        QUERY->add_ctor(QUERY, event_next_frame_ctor); // singleton
        // no destructor for singleton
        chugl_next_frame_event_data_offset
          = QUERY->add_mvar(QUERY, "int", "@cgl_next_frame_event_data", false);

        QUERY->add_mfun(QUERY, event_next_frame_waiting_on, "void",
                        "waiting_on");
        QUERY->end_class(QUERY);

        // Window resize event (TODO) ================================
        QUERY->begin_class(QUERY, "WindowResizeEvent", "Event");
        QUERY->doc_class(QUERY,
                         "Event triggered whenever the ChuGL window is "
                         "resized, either by the user or programmatically.");

        // QUERY->add_ctor(QUERY, cgl_window_resize_ctor);
        // // QUERY->add_dtor(QUERY, cgl_window_resize_dtor);

        // window_resize_event_data_offset = QUERY->add_mvar(
        //   QUERY, "int", "@cgl_window_resize_event_data", false);
        QUERY->end_class(QUERY);
    }

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

        // fps()
        // QUERY->add_sfun(QUERY, cgl_get_fps, "int", "fps");
        // QUERY->doc_func(
        //   QUERY,
        //   "FPS of current window, averaged over sliding window of 30
        //   frames");

        QUERY->end_class(QUERY); // GG
    }

    // remember
    g_chuglVM  = QUERY->ck_vm(QUERY);
    g_chuglAPI = QUERY->ck_api(QUERY);

    // wasn't that a breeze?
    return true;
}

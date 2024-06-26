#pragma once

#include <chuck/chugin.h>

#include <condition_variable>
#include <unordered_map>

#include "core/memory.h"
#include "core/spinlock.h"

static std::unordered_map<Chuck_VM_Shred*, bool> registeredShreds;
// static std::unordered_set<Chuck_VM_Shred*> waitingShreds;
static spinlock waitingShredsLock;
static u64 waitingShreds = 0;

static std::mutex gameLoopLock;
static std::condition_variable gameLoopConditionVar;
static bool shouldRender = false;

// ============================================================================
// Shared Audio/Graphics Thread State
// ============================================================================

// Window State (Don't modify directly, use API functions)
struct CHUGL_Window {
    bool closeable   = true;
    bool transparent = false;
    bool floating    = false;
    bool resizable   = true;
    bool decorated   = true;

    // window size (in screen coordinates)
    int window_width = 1280, window_height = 960;

    // framebuffer size (in pixels)
    int framebuffer_width, framebuffer_height;

    // content scale
    float content_scale_x, content_scale_y;

    // window frame size
    int window_frame_left, window_frame_top, window_frame_right,
      window_frame_bottom;

    float window_opacity;

    // locks
    spinlock window_lock;
};

CHUGL_Window chugl_window;

void CHUGL_Window_Closeable(bool closeable)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.closeable = closeable;
    spinlock::unlock(&chugl_window.window_lock);
}

bool CHUGL_Window_Closeable()
{
    spinlock::lock(&chugl_window.window_lock);
    bool closeable = chugl_window.closeable;
    spinlock::unlock(&chugl_window.window_lock);
    return closeable;
}

void CHUGL_Window_Floating(bool floating)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.floating = floating;
    spinlock::unlock(&chugl_window.window_lock);
}

bool CHUGL_Window_Floating()
{
    spinlock::lock(&chugl_window.window_lock);
    bool floating = chugl_window.floating;
    spinlock::unlock(&chugl_window.window_lock);
    return floating;
}

void CHUGL_Window_Transparent(bool transparent)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.transparent = transparent;
    spinlock::unlock(&chugl_window.window_lock);
}

bool CHUGL_Window_Transparent()
{
    spinlock::lock(&chugl_window.window_lock);
    bool transparent = chugl_window.transparent;
    spinlock::unlock(&chugl_window.window_lock);
    return transparent;
}

void CHUGL_Window_Resizable(bool resizable)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.resizable = resizable;
    spinlock::unlock(&chugl_window.window_lock);
}

bool CHUGL_Window_Resizable()
{
    spinlock::lock(&chugl_window.window_lock);
    bool resizable = chugl_window.resizable;
    spinlock::unlock(&chugl_window.window_lock);
    return resizable;
}

void CHUGL_Window_Decorated(bool decorated)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.decorated = decorated;
    spinlock::unlock(&chugl_window.window_lock);
}

bool CHUGL_Window_Decorated()
{
    spinlock::lock(&chugl_window.window_lock);
    bool decorated = chugl_window.decorated;
    spinlock::unlock(&chugl_window.window_lock);
    return decorated;
}

void CHUGL_Window_Size(int window_width, int window_height,
                       int framebuffer_width, int framebuffer_height)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.window_width       = window_width;
    chugl_window.window_height      = window_height;
    chugl_window.framebuffer_width  = framebuffer_width;
    chugl_window.framebuffer_height = framebuffer_height;
    spinlock::unlock(&chugl_window.window_lock);
}

t_CKVEC2 CHUGL_Window_WindowSize()
{
    t_CKVEC2 size;
    spinlock::lock(&chugl_window.window_lock);
    size.x = chugl_window.window_width;
    size.y = chugl_window.window_height;
    spinlock::unlock(&chugl_window.window_lock);
    return size;
}

t_CKVEC2 CHUGL_Window_FramebufferSize()
{
    t_CKVEC2 size;
    spinlock::lock(&chugl_window.window_lock);
    size.x = chugl_window.framebuffer_width;
    size.y = chugl_window.framebuffer_height;
    spinlock::unlock(&chugl_window.window_lock);
    return size;
}

void CHUGL_Window_ContentScale(float x, float y)
{
    spinlock::lock(&chugl_window.window_lock);
    chugl_window.content_scale_x = x;
    chugl_window.content_scale_y = y;
    spinlock::unlock(&chugl_window.window_lock);
}

t_CKVEC2 CHUGL_Window_ContentScale()
{
    t_CKVEC2 scale;
    spinlock::lock(&chugl_window.window_lock);
    scale.x = chugl_window.content_scale_x;
    scale.y = chugl_window.content_scale_y;
    spinlock::unlock(&chugl_window.window_lock);
    return scale;
}

// ============================================================================
// ChuGL Event API
// ============================================================================

#define CHUGL_EventTable                                                       \
    X(NEXT_FRAME = 0, "NextFrameEvent")                                        \
    X(WINDOW_RESIZE, "WindowResizeEvent")                                      \
    X(WINDOW_CLOSE, "WindowCloseEvent")                                        \
    X(CONTENT_SCALE, "ContentScaleChangedEvent")

enum CHUGL_EventType {
#define X(name, str) name,
    CHUGL_EventTable
#undef X
      CHUGL_EVENT_TYPE_COUNT
};

static const char* CHUGL_EventTypeNames[CHUGL_EVENT_TYPE_COUNT] = {
#define X(name, str) str,
    CHUGL_EventTable
#undef X
};

void Event_Broadcast(CHUGL_EventType type, CK_DL_API api, Chuck_VM* vm);
Chuck_Event* Event_Get(CHUGL_EventType type, CK_DL_API api, Chuck_VM* vm);
const char* Event_GetName(CHUGL_EventType type);

// ============================================================================
// Thread Synchronization API
// ============================================================================

bool Sync_HasShredWaited(Chuck_VM_Shred* shred);
void Sync_RegisterShred(Chuck_VM_Shred* shred);
void Sync_WaitOnUpdateDone();
void Sync_SignalUpdateDone();

// ============================================================================
// ChuGL Events Definitions
// ============================================================================

static CBufferSimple* chuckEventQueue              = NULL;
static Chuck_Event* events[CHUGL_EVENT_TYPE_COUNT] = {};

static void Event_Init(CK_DL_API api, Chuck_VM* vm)
{
    if (chuckEventQueue == NULL)
        chuckEventQueue = api->vm->create_event_buffer(vm);

    for (u32 i = 0; i < CHUGL_EVENT_TYPE_COUNT; i++) {
        if (events[i] != NULL) continue;
        Chuck_DL_Api::Type type
          = api->type->lookup(vm, CHUGL_EventTypeNames[i]);
        events[i]
          = (Chuck_Event*)api->object->create_without_shred(vm, type, true);
    }
}

void Event_Broadcast(CHUGL_EventType type, CK_DL_API api, Chuck_VM* vm)
{
    if (events[type] == NULL) Event_Init(api, vm);
    switch (type) {
        case CHUGL_EventType::NEXT_FRAME: {
            spinlock::lock(&waitingShredsLock);
            waitingShreds = 0;
            api->vm->queue_event(vm, events[type], 1, chuckEventQueue);
            spinlock::unlock(&waitingShredsLock);
            return;
        }
        default: api->vm->queue_event(vm, events[type], 1, chuckEventQueue);
    }
}

Chuck_Event* Event_Get(CHUGL_EventType type, CK_DL_API api, Chuck_VM* vm)
{
    if (events[type] == NULL) Event_Init(api, vm);
    return events[type];
}

const char* Event_GetName(CHUGL_EventType type)
{
    return CHUGL_EventTypeNames[type];
}

// ============================================================================
// Thread Synchronization Definitions
// ============================================================================

// TODO: prob want to put syncer and SG_component managers into single
// ChuGL_Context struct

static bool Sync_IsShredRegistered(Chuck_VM_Shred* shred)
{
    return registeredShreds.find(shred) != registeredShreds.end();
}

bool Sync_HasShredWaited(Chuck_VM_Shred* shred)
{
    if (!Sync_IsShredRegistered(shred)) return true; // never called before
    return registeredShreds[shred];
}

void Sync_RegisterShred(Chuck_VM_Shred* shred)
{
    registeredShreds[shred] = false;
}

void Sync_WaitOnUpdateDone()
{
    std::unique_lock<std::mutex> lock(gameLoopLock);
    gameLoopConditionVar.wait(lock, []() { return shouldRender; });
    shouldRender = false;
    // log_error("graphics thread: graphics thread woke up");
}

void Sync_SignalUpdateDone()
{
    std::unique_lock<std::mutex> lock(gameLoopLock);
    shouldRender = true;
    lock.unlock();
    gameLoopConditionVar.notify_all();
}

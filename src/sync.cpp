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

// setting physics world here now to avoid an extra frame of latency
// incurred by setting it via the command queue.

// ============================================================================
// ChuGL Event API
// ============================================================================

#define CHUGL_EventTable                                                       \
    X(NEXT_FRAME = 0, "NextFrameEvent")                                        \
    X(WINDOW_RESIZE, "WindowResizeEvent")

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
    spinlock::lock(&waitingShredsLock);
    waitingShreds = 0; // all current shreds will be woken, so reset counter
    api->vm->queue_event(vm, events[type], 1, chuckEventQueue);
    spinlock::unlock(&waitingShredsLock);
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

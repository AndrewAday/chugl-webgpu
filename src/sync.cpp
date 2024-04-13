
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "core/macros.h"

#include "sync.h"

// ============================================================================
// ChuGL Events
// ============================================================================

static const char* CHUGL_EventTypeNames[CHUGL_EVENT_TYPE_COUNT] = {
#define X(name, str) str,
    CHUGL_EventTable
#undef X
};

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
    api->vm->queue_event(vm, events[type], 1, chuckEventQueue);
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
// Thread Synchronization
// ============================================================================

static std::unordered_map<Chuck_VM_Shred*, bool> registeredShreds;
static std::unordered_set<Chuck_VM_Shred*> waitingShreds;

static std::mutex gameLoopLock;
static std::condition_variable gameLoopConditionVar;
static bool shouldRender = false;

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
}

static void _Sync_SignalUpdateDone()
{
    std::unique_lock<std::mutex> lock(gameLoopLock);
    shouldRender = true;
    lock.unlock();
    gameLoopConditionVar.notify_all();
}

void Sync_MarkShredWaited(Chuck_VM_Shred* shred)
{
    // mark shred as waited
    ASSERT(registeredShreds.find(shred) != registeredShreds.end());
    registeredShreds[shred] = true;

    // add to waiting set
    waitingShreds.insert(shred);

    // if #waiting == #registered, all chugl shreds have finished work, and we
    // are safe to wakeup the renderer
    if (waitingShreds.size() >= registeredShreds.size()) {
        // traverse scenegraph and call chuck-defined update() on all GGens
        // CGL::UpdateSceneGraph(CGL::mainScene, API, VM, SHRED);

        // clear thread waitlist; all expected shreds are now waiting on
        // GG.nextFrame()
        waitingShreds.clear();

        // signal the graphics-side that audio-side is done processing for this
        // frame see CGL::WaitOnUpdateDone() CGL::Render();
        _Sync_SignalUpdateDone();

        // Garbage collect (TODO add API function to control this)
        // Locator::GC();
    }
}

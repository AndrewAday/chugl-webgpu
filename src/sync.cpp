#pragma once

#include <chuck/chugin.h>

#include <condition_variable>
#include <unordered_map>

#include "core/memory.h"
#include "core/spinlock.h"

#include "sg_command.h"

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

void Sync_MarkShredWaited(Chuck_VM_Shred* shred);
bool Sync_HasShredWaited(Chuck_VM_Shred* shred);
void Sync_RegisterShred(Chuck_VM_Shred* shred);
void Sync_WaitOnUpdateDone();

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
// Thread Synchronization Definitions
// ============================================================================

// TODO: prob want to put syncer and SG_component managers into single
// ChuGL_Context struct

static std::unordered_map<Chuck_VM_Shred*, bool> registeredShreds;
// static std::unordered_set<Chuck_VM_Shred*> waitingShreds;
static u64 waitingShreds = 0;

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
    ++waitingShreds;

    // if #waiting == #registered, all chugl shreds have finished work, and we
    // are safe to wakeup the renderer
    // TODO: bug. If a shred does NOT call GG.nextFrame in an infinite loop,
    // i.e. does nextFrame() once and then goes on to say process audio, this
    // code will stay be expecting that shred to call nextFrame() again and
    // waitingShreds will never == registeredShreds.size() thus hanging the
    // renderer
    if (waitingShreds >= registeredShreds.size()) {
        // traverse scenegraph and call chuck-defined update() on all GGens
        // CGL::UpdateSceneGraph(CGL::mainScene, API, VM, SHRED);

        // clear thread waitlist; all expected shreds are now waiting on
        // GG.nextFrame()
        waitingShreds = 0;

        // signal the graphics-side that audio-side is done processing for this
        // frame
        _Sync_SignalUpdateDone();

        // Garbage collect (TODO add API function to control this via GG config)
        SG_GC();
    }
}

// ============================================================================
// Command Queue
// ============================================================================

static Arena _Sync_CommandQueueA = {};
static Arena _Sync_CommandQueueB = {};
// switch between the two command queues
// false means (read: A, write: B)
// true means (read: B, write: A)
static Arena* _Sync_ReadCommandQueue  = &_Sync_CommandQueueA;
static Arena* _Sync_WriteCommandQueue = &_Sync_CommandQueueB;
// command queue lock
// only held when 1: adding new command and 2:
// swapping the read/write queues
static spinlock _Sync_CommandQueueLock = {};

void Sync_Init()
{
    ASSERT(_Sync_ReadCommandQueue->base == NULL);
    ASSERT(_Sync_WriteCommandQueue->base == NULL);

    Arena::init(_Sync_ReadCommandQueue, MEGABYTE);
    Arena::init(_Sync_WriteCommandQueue, MEGABYTE);
}

void Sync_Free()
{
    Arena::free(_Sync_ReadCommandQueue);
    Arena::free(_Sync_WriteCommandQueue);
}

// swap the command queue double buffer
void Sync_SwapCommandQueues()
{
    spinlock::lock(&_Sync_CommandQueueLock);

    // assert write queue has been flushed before swapping
    ASSERT(_Sync_WriteCommandQueue->curr == 0);

    // swap queues
    Arena* temp             = _Sync_ReadCommandQueue;
    _Sync_ReadCommandQueue  = _Sync_WriteCommandQueue;
    _Sync_WriteCommandQueue = temp;

    spinlock::unlock(&_Sync_CommandQueueLock);
}

bool Sync_ReadCommandQueueIter(SG_Command** command)
{
    // command queue empty
    if (_Sync_ReadCommandQueue->curr == 0) {
        *command = NULL;
        return false;
    }

    // return the first command in queue
    if (*command == NULL) {
        *command = (SG_Command*)_Sync_ReadCommandQueue->base;
        return true;
    }

    // sanity bounds check
    ASSERT(*command >= (SG_Command*)_Sync_ReadCommandQueue->base
           && *command < (SG_Command*)Arena::top(_Sync_ReadCommandQueue));

    // at last element
    if ((*command)->nextCommandOffset == _Sync_ReadCommandQueue->curr) {
        *command = NULL;
        return false;
    }

    // else return the nextOffset
    *command = (SG_Command*)Arena::get(_Sync_ReadCommandQueue,
                                       (*command)->nextCommandOffset);
    return true;
}

void Sync_ReadCommandQueueClear()
{
    Arena::clear(_Sync_ReadCommandQueue);
}

// void Sync_Push(SG_CommandType type, void* data)
// {
//     const size_t size                 = SG_CommandSizes[type];
//     const SG_CommandInitFunc initFunc = SG_CommandInitFuncs[type];

//     spinlock::lock(&_Sync_CommandQueueLock);

//     // allocate memory
//     SG_Command* command
//       = (SG_Command*)Arena::push(_Sync_WriteCommandQueue, size);

//     // initialize memory
//     command->type              = type;
//     command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
//     initFunc(command, data);

//     spinlock::unlock(&_Sync_CommandQueueLock);
// }

void Sync_PushCommand_CreateTransform(Chuck_Object* ckobj,
                                      t_CKUINT component_offset_id,
                                      CK_DL_API API)
{
    // execute change on audio thread side
    SG_Transform* xform = SG_CreateTransform(ckobj);
    // save SG_ID
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = xform->id;

    spinlock::lock(&_Sync_CommandQueueLock);
    {
        // allocate memory
        SG_Command_CreateXform* command
          = ARENA_PUSH_TYPE(_Sync_WriteCommandQueue, SG_Command_CreateXform);

        // initialize memory
        command->type              = SG_COMMAND_TYPE_CREATE_XFORM;
        command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
        command->sg_id             = xform->id;
        command->pos               = xform->pos;
        command->rot               = xform->rot;
        command->sca               = xform->sca;
    }
    spinlock::unlock(&_Sync_CommandQueueLock);
}

void Sync_PushCommand_AddChild(SG_Transform* parent, SG_Transform* child)
{
    // execute change on audio thread side
    SG_Transform::addChild(parent, child);

    spinlock::lock(&_Sync_CommandQueueLock);
    {
        // allocate memory
        SG_Command_AddChild* command
          = ARENA_PUSH_TYPE(_Sync_WriteCommandQueue, SG_Command_AddChild);

        // initialize memory
        command->type              = SG_COMMAND_TYPE_ADD_CHILD;
        command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
        command->parent_id         = parent->id;
        command->child_id          = child->id;
    }
    spinlock::unlock(&_Sync_CommandQueueLock);
}

void Sync_PushCommand_RemoveChild(SG_Transform* parent, SG_Transform* child)
{
    // execute change on audio thread side
    SG_Transform::removeChild(parent, child);

    spinlock::lock(&_Sync_CommandQueueLock);
    {
        // allocate memory
        SG_Command_RemoveChild* command
          = ARENA_PUSH_TYPE(_Sync_WriteCommandQueue, SG_Command_RemoveChild);

        // initialize memory
        command->type              = SG_COMMAND_TYPE_REMOVE_CHILD;
        command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
        command->parent            = parent->id;
        command->child             = child->id;
    }
    spinlock::unlock(&_Sync_CommandQueueLock);
}

void Sync_PushCommand_SetPosition(SG_Transform* xform)
{
    spinlock::lock(&_Sync_CommandQueueLock);
    {
        // allocate memory
        SG_Command_SetPosition* command
          = ARENA_PUSH_TYPE(_Sync_WriteCommandQueue, SG_Command_SetPosition);

        // initialize memory
        command->type              = SG_COMMAND_TYPE_SET_POSITION;
        command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
        command->sg_id             = xform->id;
        command->pos               = xform->pos;
    }
    spinlock::unlock(&_Sync_CommandQueueLock);
}

void Sync_PushCommand_SetRotation(SG_Transform* xform)
{
    spinlock::lock(&_Sync_CommandQueueLock);
    {
        // allocate memory
        SG_Command_SetRotation* command
          = ARENA_PUSH_TYPE(_Sync_WriteCommandQueue, SG_Command_SetRotation);

        // initialize memory
        command->type              = SG_COMMAND_TYPE_SET_ROTATATION;
        command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
        command->sg_id             = xform->id;
        command->rot               = xform->rot;
    }
    spinlock::unlock(&_Sync_CommandQueueLock);
}

void Sync_PushCommand_SetScale(SG_Transform* xform)
{
    spinlock::lock(&_Sync_CommandQueueLock);
    {
        // allocate memory
        SG_Command_SetScale* command
          = ARENA_PUSH_TYPE(_Sync_WriteCommandQueue, SG_Command_SetScale);

        // initialize memory
        command->type              = SG_COMMAND_TYPE_SET_SCALE;
        command->nextCommandOffset = _Sync_WriteCommandQueue->curr;
        command->sg_id             = xform->id;
        command->sca               = xform->sca;
    }
    spinlock::unlock(&_Sync_CommandQueueLock);
}

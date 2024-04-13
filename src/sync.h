#pragma once

#include <chuck/chugin.h>

// ============================================================================
// ChuGL Event API
// ============================================================================

struct Chuck_Event;
struct Chuck_VM;
struct Chuck_DL_Api;
typedef const Chuck_DL_Api* CK_DL_API;

#define CHUGL_EventTable                                                       \
    X(NEXT_FRAME, "NextFrameEvent")                                            \
    X(WINDOW_RESIZE, "WindowResizeEvent")

enum CHUGL_EventType {
#define X(name, str) name,
    CHUGL_EventTable
#undef X
      CHUGL_EVENT_TYPE_COUNT
};

void Event_Broadcast(CHUGL_EventType type, CK_DL_API api, Chuck_VM* vm);
Chuck_Event* Event_Get(CHUGL_EventType type, CK_DL_API api, Chuck_VM* vm);
const char* Event_GetName(CHUGL_EventType type);

// ============================================================================
// Thread Synchronization
// ============================================================================

void Sync_MarkShredWaited(Chuck_VM_Shred* shred);
bool Sync_HasShredWaited(Chuck_VM_Shred* shred);
void Sync_RegisterShred(Chuck_VM_Shred* shred);

void Sync_WaitOnUpdateDone();
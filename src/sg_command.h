#pragma once

#include <stddef.h>

#include "core/macros.h"
#include "sg_component.h"

// #define SG_COMMAND_STRUCT(type) struct type##_Data
// #define SG_COMMAND_DATA_PTR(type) type##_Data*

/*
What needs to happen?

All writes to writeQueue must be lock protected.

What needs to be known:
- how much memory to allocate:
    - command type (enum)  1:1 with command struct
    - table from command type to size
- how to initialize hte memory
    - table from command type to init function
{
    execute command on audio thread
    THEN
    locked {
        allocate memory for new command
        initialize memory
    } // unlock
}

Do I (1) initialize in separate memory and then copy over,
or do I (2) initialize in the writequeue memory?
- probably (2), (1) requires a separate audio-thread frame arena and won't
necessarily be faster
- tradeoff: all memory is contiguous, no chasing down pointers (prev
implementation was pointer array to commands). But means holding the commandQ
lock for longer. Contention increases, renderer may be blocked on swapQueue. but
my guess is that as # of commands inceases, time saved from avoiding cache
misses will outweigh the lock contention. Also it's okay for the renderer to be
blocked in favor of saving time on the audio thread. (audio thread doesn't need
to ever allocate memory itself or do twice the work of initializing in some
frame arena and then copy over)
*/

enum SG_CommandType : u32 {
    SG_COMMAND_NONE = 0,
    SG_COMMAND_GG_SCENE,
    SG_COMMAND_CREATE_XFORM,
    SG_COMMAND_ADD_CHILD,
    SG_COMMAND_REMOVE_CHILD,
    SG_COMMAND_SET_POSITION,
    SG_COMMAND_SET_ROTATATION,
    SG_COMMAND_SET_SCALE,
    SG_COMMAND_SCENE_CREATE,
    SG_COMMAND_SCENE_BG_COLOR,
    SG_COMMAND_GEO_CREATE,
    SG_COMMAND_MATERIAL_CREATE,
    SG_COMMAND_COUNT
};

struct SG_Command {
    SG_CommandType type;
    u64 nextCommandOffset;
};

struct SG_Command_GG_Scene : public SG_Command {
    SG_ID sg_id;
};

struct SG_Command_CreateXform : public SG_Command {
    SG_ID sg_id;
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 sca;

    // relationships
    // SG_ID parentID;
};

struct SG_Command_AddChild : public SG_Command {
    SG_ID parent_id;
    SG_ID child_id;
};

struct SG_Command_RemoveChild : public SG_Command {
    SG_ID parent;
    SG_ID child;
};

struct SG_Command_SetPosition : public SG_Command {
    SG_ID sg_id;
    glm::vec3 pos;
};

struct SG_Command_SetRotation : public SG_Command {
    SG_ID sg_id;
    glm::quat rot;
};

struct SG_Command_SetScale : public SG_Command {
    SG_ID sg_id;
    glm::vec3 sca;
};

struct SG_Command_SceneCreate : public SG_Command {
    SG_ID sg_id;
};

struct SG_Command_SceneBGColor : public SG_Command {
    SG_ID sg_id;
    glm::vec4 color;
};

struct SG_Command_GeoCreate : public SG_Command {
    SG_ID sg_id;
    SG_GeometryParams params;
    SG_GeometryType geo_type;
};

// TODO: need way to specify config (doublesided, alpha, etc)
struct SG_Command_MaterialCreate : public SG_Command {
    SG_ID sg_id;
    SG_MaterialParams params;
    SG_MaterialType material_type;
};

// ============================================================================
// Command Queue API
// ============================================================================

void CQ_Init();
void CQ_Free();

// swap the command queue double buffer
void CQ_SwapQueues();

bool CQ_ReadCommandQueueIter(SG_Command** command);

void CQ_ReadCommandQueueClear();

// ============================================================================
// Commands
// ============================================================================

void CQ_PushCommand_GG_Scene(SG_Scene* scene);
void CQ_PushCommand_CreateTransform(Chuck_Object* ckobj,
                                    t_CKUINT component_offset_id,
                                    CK_DL_API API);
void CQ_PushCommand_AddChild(SG_Transform* parent, SG_Transform* child);
void CQ_PushCommand_RemoveChild(SG_Transform* parent, SG_Transform* child);
void CQ_PushCommand_SetPosition(SG_Transform* xform);
void CQ_PushCommand_SetRotation(SG_Transform* xform);
void CQ_PushCommand_SetScale(SG_Transform* xform);

SG_Scene* CQ_PushCommand_SceneCreate(Chuck_Object* ckobj,
                                     t_CKUINT component_offset_id,
                                     CK_DL_API API);

void CQ_PushCommand_SceneBGColor(SG_Scene* scene, t_CKVEC4 color);
void CQ_PushCommand_GeometryCreate(SG_Geometry* geo);

void CQ_PushCommand_MaterialCreate(SG_Material* material);
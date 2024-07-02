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
    // window
    SG_COMMAND_WINDOW_CLOSE,
    SG_COMMAND_WINDOW_MODE,
    SG_COMMAND_WINDOW_SIZE_LIMITS,
    SG_COMMAND_WINDOW_POSITION,
    SG_COMMAND_WINDOW_CENTER,
    SG_COMMAND_WINDOW_TITLE,
    SG_COMMAND_WINDOW_ICONIFY,
    SG_COMMAND_WINDOW_ATTRIBUTE,
    SG_COMMAND_WINDOW_OPACITY,

    // mouse
    SG_COMMAND_MOUSE_MODE,
    SG_COMMAND_MOUSE_CURSOR,
    SG_COMMAND_MOUSE_CURSOR_NORMAL,

    // UI
    SG_COMMAND_UI_DISABLED,

    // b2 physics
    SG_COMMAND_b2_WORLD_SET,
    SG_COMMAND_b2_SUBSTEP_COUNT, // # of substeps per physics step

    // components
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
    SG_COMMAND_MESH_CREATE,
    SG_COMMAND_COUNT
};

struct SG_Command {
    SG_CommandType type;
    u64 nextCommandOffset;
};

// Window Commands --------------------------------------------------------

struct SG_Command_WindowClose : public SG_Command {
};

enum SG_WindowMode : u8 {
    SG_WINDOW_MODE_WINDOWED = 0,
    SG_WINDOW_MODE_FULLSCREEN,
    SG_WINDOW_MODE_WINDOWED_FULLSCREEN
};

enum CHUGL_WindowAttrib : u8 {
    CHUGL_WINDOW_ATTRIB_RESIZABLE = 0,
    CHUGL_WINDOW_ATTRIB_DECORATED,
    CHUGL_WINDOW_ATTRIB_FLOATING,
    CHUGL_WINDOW_ATTRIB_TRANSPARENT_FRAMEBUFFER,
};

struct SG_Command_WindowMode : public SG_Command {
    SG_WindowMode mode;
    int width;
    int height;
};

struct SG_Command_WindowSizeLimits : public SG_Command {
    int min_width;
    int min_height;
    int max_width;
    int max_height;
    int aspect_ratio_x;
    int aspect_ratio_y;
};

struct SG_Command_WindowPosition : public SG_Command {
    int x;
    int y;
};

struct SG_Command_WindowCenter : public SG_Command {
};

struct SG_Command_WindowTitle : public SG_Command {
    u64 title_offset; // get char* via CG_ReadCommandGetOffset(title_offset)
};

struct SG_Command_WindowIconify : public SG_Command {
    bool iconify;
};

struct SG_Command_WindowAttribute : public SG_Command {
    CHUGL_WindowAttrib attrib;
    bool value;
};

struct SG_Command_WindowOpacity : public SG_Command {
    float opacity;
};

// Mouse commands ---------------------------------------------------------

struct SG_Command_MouseMode : public SG_Command {
    int mode;
};

struct SG_Command_MouseCursorNormal : public SG_Command {
};

struct SG_Command_MouseCursor : public SG_Command {
    u64 mouse_cursor_image_offset;
    u32 width;
    u32 height;
    u32 xhot;
    u32 yhot;
};

// UI commands ---------------------------------------------------------

struct SG_Command_UI_Disabled : public SG_Command {
    bool disabled;
};

// Component Commands -----------------------------------------------------

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

struct SG_Command_Mesh_Create : public SG_Command {
    SG_ID mesh_id; // gmesh id
    SG_ID geo_id;
    SG_ID mat_id;
};

struct SG_Command_b2World_Set : public SG_Command {
    u32 b2_world_id;
};

struct SG_Command_b2_SubstepCount : public SG_Command {
    u32 substep_count;
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

// some command structs have variable data (e.g. strings), which are stored in
// the same command queue arena as cmd->xxx_offset. This function returns the
// pointer to the data at the offset.
// (necessary to avoid segfaults from direct pointers to the arena memory
// caused by Arena resizing)
void* CQ_ReadCommandGetOffset(u64 byte_offset);

// ============================================================================
// Commands
// ============================================================================

// window ---------------------------------------------------------------

void CQ_PushCommand_WindowClose();
void CQ_PushCommand_WindowMode(SG_WindowMode mode, int width, int height);
void CQ_PushCommand_WindowSizeLimits(int min_width, int min_height,
                                     int max_width, int max_height,
                                     int aspect_ratio_x, int aspect_ratio_y);
void CQ_PushCommand_WindowPosition(int x, int y);
void CQ_PushCommand_WindowCenter();
void CQ_PushCommand_WindowTitle(const char* title);
void CQ_PushCommand_WindowIconify(bool iconify);
void CQ_PushCommand_WindowAttribute(CHUGL_WindowAttrib attrib, bool value);
void CQ_PushCommand_WindowOpacity(float opacity);

void CQ_PushCommand_MouseMode(int mode);
void CQ_PushCommand_MouseCursor(CK_DL_API API, Chuck_ArrayInt* image_data,
                                u32 width, u32 height, u32 xhot, u32 yhot);

void CQ_PushCommand_MouseCursorNormal();

// UI -------------------------------------------------------------------
void CQ_PushCommand_UI_Disabled(bool disabled);

// components

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
void CQ_PushCommand_Mesh_Create(SG_Mesh* mesh);

// b2
void CQ_PushCommand_b2World_Set(u32 world_id);
void CQ_PushCommand_b2SubstepCount(u32 substep_count);
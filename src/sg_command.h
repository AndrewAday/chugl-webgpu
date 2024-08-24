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
    SG_COMMAND_SCENE_SET_MAIN_CAMERA,

    // shader
    SG_COMMAND_SHADER_CREATE,

    // material
    SG_COMMAND_MATERIAL_CREATE,
    SG_COMMAND_MATERIAL_UPDATE_PSO,
    SG_COMMAND_MATERIAL_SET_UNIFORM,
    SG_COMMAND_MATERIAL_SET_STORAGE_BUFFER,
    SG_COMMAND_MATERIAL_SET_SAMPLER,
    SG_COMMAND_MATERIAL_SET_TEXTURE,

    // mesh
    SG_COMMAND_MESH_CREATE,

    // camera
    SG_COMMAND_CAMERA_CREATE,
    SG_COMMAND_CAMERA_SET_PARAMS,

    // text
    SG_COMMAND_TEXT_REBUILD,
    SG_COMMAND_TEXT_DEFAULT_FONT,

    // text
    SG_COMMAND_PASS_CREATE,
    SG_COMMAND_PASS_UPDATE,
    SG_COMMAND_PASS_CONNECT,
    SG_COMMAND_PASS_DISCONNECT,

    // geometry
    SG_COMMAND_GEO_CREATE,
    SG_COMMAND_GEO_SET_VERTEX_ATTRIBUTE,
    SG_COMMAND_GEO_SET_PULLED_VERTEX_ATTRIBUTE,
    SG_COMMAND_GEO_SET_VERTEX_COUNT,
    SG_COMMAND_GEO_SET_INDICES,

    // texture
    SG_COMMAND_TEXTURE_CREATE,
    SG_COMMAND_TEXTURE_DATA,
    SG_COMMAND_TEXTURE_FROM_FILE,

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

struct SG_Command_SceneSetMainCamera : public SG_Command {
    SG_ID scene_id;
    SG_ID camera_id;
};

struct SG_Command_GeoCreate : public SG_Command {
    SG_ID sg_id;
    SG_GeometryParams params;
    SG_GeometryType geo_type;
};

struct SG_Command_GeoSetVertexAttribute : public SG_Command {
    SG_ID sg_id;
    int location;
    int num_components;
    int data_len;          // # of floats in data array
    ptrdiff_t data_offset; // byte offset into command queue arena for attribute data
};

struct SG_Command_GeometrySetPulledVertexAttribute : public SG_Command {
    SG_ID sg_id;
    int location;
    size_t data_bytes;
    ptrdiff_t data_offset;
};

struct SG_Command_GeometrySetVertexCount : public SG_Command {
    SG_ID sg_id;
    int count;
};

struct SG_Command_GeoSetIndices : public SG_Command {
    SG_ID sg_id;
    int index_count;
    ptrdiff_t indices_offset;
};

struct SG_Command_TextureCreate : public SG_Command {
    SG_ID sg_id;
    // texture descriptor
    bool is_storage = false;
};

struct SG_Command_TextureData : public SG_Command {
    SG_ID sg_id;
    int width; // for now bytes per row is always width * 4
    int height;
    ptrdiff_t data_offset;
};

struct SG_Command_TextureFromFile : public SG_Command {
    SG_ID sg_id;
    ptrdiff_t filepath_offset;
};

struct SG_Command_ShaderCreate : public SG_Command {
    SG_ID sg_id;
    // strings to be freed by render thread
    ptrdiff_t vertex_string_offset;
    ptrdiff_t vertex_filepath_offset;
    ptrdiff_t fragment_string_offset;
    ptrdiff_t fragment_filepath_offset;
    WGPUVertexFormat vertex_layout[SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES];
};

struct SG_Command_MaterialCreate : public SG_Command {
    SG_ID sg_id;
    // SG_MaterialParams params;
    SG_MaterialType material_type;
    SG_MaterialPipelineState pso;
};

struct SG_Command_MaterialUpdatePSO : public SG_Command {
    SG_ID sg_id;
    SG_MaterialPipelineState pso;
};

struct SG_Command_MaterialSetUniform : public SG_Command {
    SG_ID sg_id;
    SG_MaterialUniform uniform;
    int location;
};

struct SG_Command_MaterialSetStorageBuffer : public SG_Command {
    SG_ID sg_id;
    int location;
    ptrdiff_t data_offset;
    int data_size_bytes;
};

struct SG_Command_MaterialSetSampler : public SG_Command {
    SG_ID sg_id;
    int location;
    SG_Sampler sampler;
};

struct SG_Command_MaterialSetTexture : public SG_Command {
    SG_ID sg_id;
    int location;
    SG_ID texture_id;
};

struct SG_Command_Mesh_Create : public SG_Command {
    SG_ID mesh_id; // gmesh id
    SG_ID geo_id;
    SG_ID mat_id;
};

// text commands -----------------------------------------------------

struct SG_Command_TextDefaultFont : public SG_Command {
    ptrdiff_t font_path_str_offset;
};

struct SG_Command_TextRebuild : public SG_Command {
    SG_ID text_id; // lazily create text if not found
    SG_ID material_id;
    glm::vec2 control_point; // TODO do as material uniform
    float vertical_spacing;
    ptrdiff_t font_path_str_offset;
    ptrdiff_t text_str_offset;
};

// camera commands -----------------------------------------------------

struct SG_Command_CameraCreate : public SG_Command {
    SG_Camera camera;
};

struct SG_Command_CameraSetParams : public SG_Command {
    SG_ID camera_id;
    SG_CameraParams params;
};

// pass commands -----------------------------------------------------

struct SG_Command_PassCreate : public SG_Command {
    SG_ID pass_id;
    SG_PassType pass_type;
};

// TODO consolidate into single struct, copy all of SG_Pass?
struct SG_Command_PassUpdate : public SG_Command {
    SG_Pass pass;
};

struct SG_Command_PassConnect : public SG_Command {
    SG_ID pass_id;
    SG_ID next_pass_id;
};

// b2 physics commands -----------------------------------------------------

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
void CQ_PushCommand_WindowSizeLimits(int min_width, int min_height, int max_width,
                                     int max_height, int aspect_ratio_x,
                                     int aspect_ratio_y);
void CQ_PushCommand_WindowPosition(int x, int y);
void CQ_PushCommand_WindowCenter();
void CQ_PushCommand_WindowTitle(const char* title);
void CQ_PushCommand_WindowIconify(bool iconify);
void CQ_PushCommand_WindowAttribute(CHUGL_WindowAttrib attrib, bool value);
void CQ_PushCommand_WindowOpacity(float opacity);

void CQ_PushCommand_MouseMode(int mode);
void CQ_PushCommand_MouseCursor(CK_DL_API API, Chuck_ArrayInt* image_data, u32 width,
                                u32 height, u32 xhot, u32 yhot);

void CQ_PushCommand_MouseCursorNormal();

// UI -------------------------------------------------------------------
void CQ_PushCommand_UI_Disabled(bool disabled);

// components

void CQ_PushCommand_GG_Scene(SG_Scene* scene);
void CQ_PushCommand_CreateTransform(Chuck_Object* ckobj, t_CKUINT component_offset_id,
                                    CK_DL_API API);
void CQ_PushCommand_AddChild(SG_Transform* parent, SG_Transform* child);
void CQ_PushCommand_RemoveChild(SG_Transform* parent, SG_Transform* child);
void CQ_PushCommand_SetPosition(SG_Transform* xform);
void CQ_PushCommand_SetRotation(SG_Transform* xform);
void CQ_PushCommand_SetScale(SG_Transform* xform);

SG_Scene* CQ_PushCommand_SceneCreate(Chuck_Object* ckobj, t_CKUINT component_offset_id,
                                     CK_DL_API API);

void CQ_PushCommand_SceneBGColor(SG_Scene* scene, t_CKVEC4 color);
void CQ_PushCommand_SceneSetMainCamera(SG_Scene* scene, SG_Camera* camera);

// geometry
void CQ_PushCommand_GeometryCreate(SG_Geometry* geo);
void CQ_PushCommand_GeometrySetVertexAttribute(SG_Geometry* geo, int location,
                                               int num_components, f32* data,
                                               int data_len);
void CQ_PushCommand_GeometrySetIndices(SG_Geometry* geo, u32* indices, int index_count);
void CQ_PushCommand_GeometrySetPulledVertexAttribute(SG_Geometry* geo, int location,
                                                     void* data, size_t bytes);
void CQ_PushCommand_GeometrySetVertexCount(SG_Geometry* geo, int count);

// texture
void CQ_PushCommand_TextureCreate(SG_Texture* texture, bool is_storage);
void CQ_PushCommand_TextureData(
  SG_Texture* texture); // TODO currently assumes texture data is already set

void CQ_PushCommand_TextureFromFile(SG_Texture* texture, const char* filepath);

// shader
void CQ_PushCommand_ShaderCreate(SG_Shader* shader);

// material
void CQ_PushCommand_MaterialCreate(SG_Material* material);
void CQ_PushCommand_MaterialUpdatePSO(SG_Material* material);
void CQ_PushCommand_MaterialSetUniform(SG_Material* material, int location);

void CQ_PushCommand_MaterialSetStorageBuffer(SG_Material* material, int location,
                                             Chuck_ArrayFloat* ck_arr);

void CQ_PushCommand_MaterialSetSampler(SG_Material* material, int location,
                                       SG_Sampler sampler);
void CQ_PushCommand_MaterialSetTexture(SG_Material* material, int location,
                                       SG_Texture* texture);

// mesh
void CQ_PushCommand_Mesh_Create(SG_Mesh* mesh);

// camera
void CQ_PushCommand_CameraCreate(SG_Camera* camera);
void CQ_PushCommand_CameraSetParams(SG_Camera* camera);

// text
void CQ_PushCommand_TextRebuild(SG_Text* text);
void CQ_PushCommand_TextDefaultFont(const char* font_path);

// pass
// void CQ_PushCommand_PassCreate(SG_Pass* pass);
void CQ_PushCommand_PassUpdate(SG_Pass* pass);
void CQ_PushCommand_PassConnect(SG_Pass* pass, SG_Pass* next_pass);
void CQ_PushCommand_PassDisconnect(SG_Pass* pass, SG_Pass* next_pass);

// b2
void CQ_PushCommand_b2World_Set(u32 world_id);
void CQ_PushCommand_b2SubstepCount(u32 substep_count);
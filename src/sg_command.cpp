#include "sg_command.h"

#include "core/macros.h"
#include "core/spinlock.h"

// include for static assert
#include <type_traits>

// command queue
struct CQ {
    // command queue lock
    // only held when 1: adding new command and 2:
    // swapping the read/write queues
    spinlock write_q_lock;
    Arena* read_q;
    Arena* write_q;

    // TODO: test if toggle is faster
    // brings the command queue struct to below 64 bytes
    // b8 toggle;

    Arena cq_a = {};
    Arena cq_b = {};
};

// enforce it fits within a cache line
// static_assert(sizeof(CQ) <= 64);
// TODO: look into aligned_alloc for cache line alignment

static CQ cq = {};

// static Arena* _CQ_GetReadQ()
// {
//     return cq.toggle ? &cq.cq_a : &cq.cq_b;
// }

// static Arena* _CQ_GetWriteQ()
// {
//     return cq.toggle ? &cq.cq_b : &cq.cq_a;
// }

void CQ_Init()
{
    ASSERT(cq.cq_a.base == NULL);
    ASSERT(cq.cq_b.base == NULL);

    Arena::init(&cq.cq_a, MEGABYTE);
    Arena::init(&cq.cq_b, MEGABYTE);

    cq.read_q  = &cq.cq_a;
    cq.write_q = &cq.cq_b;
}

void CQ_Free()
{
    Arena::free(&cq.cq_a);
    Arena::free(&cq.cq_b);
}

// swap the command queue double buffer
void CQ_SwapQueues()
{
    // assert read queue has been flushed before swapping
    ASSERT(cq.read_q->curr == 0);

    Arena* temp = cq.read_q;
    cq.read_q   = cq.write_q;

    spinlock::lock(&cq.write_q_lock);
    cq.write_q = temp;
    spinlock::unlock(&cq.write_q_lock);
}

bool CQ_ReadCommandQueueIter(SG_Command** command)
{
    // command queue empty
    if (cq.read_q->curr == 0) {
        *command = NULL;
        return false;
    }

    // return the first command in queue
    if (*command == NULL) {
        *command = (SG_Command*)cq.read_q->base;
        return true;
    }

    // sanity bounds check
    ASSERT(*command >= (SG_Command*)cq.read_q->base
           && *command < (SG_Command*)Arena::top(cq.read_q));

    // at last element
    if ((*command)->nextCommandOffset == cq.read_q->curr) {
        *command = NULL;
        return false;
    }

    // else return the nextOffset
    *command = (SG_Command*)Arena::get(cq.read_q, (*command)->nextCommandOffset);
    return true;
}

void CQ_ReadCommandQueueClear()
{
    Arena::clear(cq.read_q);
}

void* CQ_ReadCommandGetOffset(u64 byte_offset)
{
    return Arena::get(cq.read_q, byte_offset);
}

// ============================================================================
// Command API
// ============================================================================

#define BEGIN_COMMAND(cmd_type, cmd_enum)                                              \
    spinlock::lock(&cq.write_q_lock);                                                  \
    cmd_type* command = ARENA_PUSH_TYPE(cq.write_q, cmd_type);                         \
    command->type     = cmd_enum;

#define END_COMMAND()                                                                  \
    command->nextCommandOffset = cq.write_q->curr;                                     \
    spinlock::unlock(&cq.write_q_lock);

void CQ_PushCommand_WindowClose()
{
    BEGIN_COMMAND(SG_Command_WindowClose, SG_COMMAND_WINDOW_CLOSE);
    END_COMMAND();
}

void CQ_PushCommand_WindowMode(SG_WindowMode mode, int width, int height)
{
    BEGIN_COMMAND(SG_Command_WindowMode, SG_COMMAND_WINDOW_MODE);
    command->mode   = mode;
    command->width  = width;
    command->height = height;
    END_COMMAND();
}

void CQ_PushCommand_WindowSizeLimits(int min_width, int min_height, int max_width,
                                     int max_height, int aspect_ratio_x,
                                     int aspect_ratio_y)
{
    BEGIN_COMMAND(SG_Command_WindowSizeLimits, SG_COMMAND_WINDOW_SIZE_LIMITS);
    command->min_width      = min_width;
    command->min_height     = min_height;
    command->max_width      = max_width;
    command->max_height     = max_height;
    command->aspect_ratio_x = aspect_ratio_x;
    command->aspect_ratio_y = aspect_ratio_y;
    END_COMMAND();
}

void CQ_PushCommand_WindowPosition(int x, int y)
{
    BEGIN_COMMAND(SG_Command_WindowPosition, SG_COMMAND_WINDOW_POSITION);
    command->x = x;
    command->y = y;
    END_COMMAND();
}

void CQ_PushCommand_WindowCenter()
{
    BEGIN_COMMAND(SG_Command_WindowPosition, SG_COMMAND_WINDOW_CENTER);
    END_COMMAND();
}

// copies title into command arena
void CQ_PushCommand_WindowTitle(const char* title)
{
    spinlock::lock(&cq.write_q_lock);
    SG_Command_WindowTitle* command
      = ARENA_PUSH_TYPE(cq.write_q, SG_Command_WindowTitle);
    size_t title_len = strlen(title) + 1;
    // push bytes for title
    char* title_copy = (char*)Arena::pushZero(cq.write_q, title_len);
    // copy title
    strncpy(title_copy, title, title_len - 1);

    // store offset not pointer in case arena resizes
    command->title_offset = Arena::offsetOf(cq.write_q, title_copy);

    command->type              = SG_COMMAND_WINDOW_TITLE;
    command->nextCommandOffset = cq.write_q->curr;

    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_WindowIconify(bool iconify)
{
    BEGIN_COMMAND(SG_Command_WindowIconify, SG_COMMAND_WINDOW_ICONIFY);
    command->iconify = iconify;
    END_COMMAND();
}

void CQ_PushCommand_WindowAttribute(CHUGL_WindowAttrib attrib, bool value)
{
    BEGIN_COMMAND(SG_Command_WindowAttribute, SG_COMMAND_WINDOW_ATTRIBUTE);
    command->attrib = attrib;
    command->value  = value;
    END_COMMAND();
}

void CQ_PushCommand_WindowOpacity(float opacity)
{
    BEGIN_COMMAND(SG_Command_WindowOpacity, SG_COMMAND_WINDOW_OPACITY);
    command->opacity = opacity;
    END_COMMAND();
}

void CQ_PushCommand_MouseMode(int mode)
{
    BEGIN_COMMAND(SG_Command_MouseMode, SG_COMMAND_MOUSE_MODE);
    command->mode = mode;
    END_COMMAND();
}

void CQ_PushCommand_MouseCursorNormal()
{
    BEGIN_COMMAND(SG_Command_MouseCursorNormal, SG_COMMAND_MOUSE_CURSOR_NORMAL);
    END_COMMAND();
}

void CQ_PushCommand_MouseCursor(CK_DL_API API, Chuck_ArrayInt* image_data, u32 width,
                                u32 height, u32 xhot, u32 yhot)
{
    u32 data_size = API->object->array_int_size(image_data);
    ASSERT(data_size == width * height * 4);

    spinlock::lock(&cq.write_q_lock);

    SG_Command_MouseCursor* command
      = ARENA_PUSH_TYPE(cq.write_q, SG_Command_MouseCursor);

    // push bytes for pixel data
    char* image_data_bytes = (char*)Arena::push(cq.write_q, data_size);
    // copy
    for (u32 i = 0; i < data_size; i++) {
        image_data_bytes[i]
          = (unsigned char)CLAMP(API->object->array_int_get_idx(image_data, i), 0, 255);
    }
    // store offset not pointer in case arena resizes
    command->mouse_cursor_image_offset = Arena::offsetOf(cq.write_q, image_data_bytes);
    command->width                     = width;
    command->height                    = height;
    command->xhot                      = xhot;
    command->yhot                      = yhot;
    command->type                      = SG_COMMAND_MOUSE_CURSOR;
    command->nextCommandOffset         = cq.write_q->curr;

    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_UI_Disabled(bool disabled)
{
    BEGIN_COMMAND(SG_Command_UI_Disabled, SG_COMMAND_UI_DISABLED);
    command->disabled = disabled;
    END_COMMAND();
}

void CQ_PushCommand_GG_Scene(SG_Scene* scene)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_GG_Scene* command = ARENA_PUSH_TYPE(cq.write_q, SG_Command_GG_Scene);

        // initialize memory
        command->type              = SG_COMMAND_GG_SCENE;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = scene ? scene->id : 0;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_CreateTransform(Chuck_Object* ckobj, t_CKUINT component_offset_id,
                                    CK_DL_API API)
{
    // execute change on audio thread side
    SG_Transform* xform = SG_CreateTransform(ckobj);
    // save SG_ID
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = xform->id;

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_CreateXform* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_CreateXform);

        // initialize memory
        command->type              = SG_COMMAND_CREATE_XFORM;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->pos               = xform->pos;
        command->rot               = xform->rot;
        command->sca               = xform->sca;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_AddChild(SG_Transform* parent, SG_Transform* child)
{
    // execute change on audio thread side
    SG_Transform::addChild(parent, child);

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_AddChild* command = ARENA_PUSH_TYPE(cq.write_q, SG_Command_AddChild);

        // initialize memory
        command->type              = SG_COMMAND_ADD_CHILD;
        command->nextCommandOffset = cq.write_q->curr;
        command->parent_id         = parent->id;
        command->child_id          = child->id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_RemoveChild(SG_Transform* parent, SG_Transform* child)
{
    // execute change on audio thread side
    SG_Transform::removeChild(parent, child);

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_RemoveChild* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_RemoveChild);

        // initialize memory
        command->type              = SG_COMMAND_REMOVE_CHILD;
        command->nextCommandOffset = cq.write_q->curr;
        command->parent            = parent->id;
        command->child             = child->id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SetPosition(SG_Transform* xform)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SetPosition* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetPosition);

        // initialize memory
        command->type              = SG_COMMAND_SET_POSITION;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->pos               = xform->pos;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SetRotation(SG_Transform* xform)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SetRotation* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetRotation);

        // initialize memory
        command->type              = SG_COMMAND_SET_ROTATATION;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->rot               = xform->rot;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SetScale(SG_Transform* xform)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SetScale* command = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetScale);

        // initialize memory
        command->type              = SG_COMMAND_SET_SCALE;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->sca               = xform->sca;
    }
    spinlock::unlock(&cq.write_q_lock);
}

SG_Scene* CQ_PushCommand_SceneCreate(Chuck_Object* ckobj, t_CKUINT component_offset_id,
                                     CK_DL_API API)
{
    // execute change on audio thread side
    SG_Scene* scene = SG_CreateScene(ckobj);
    // save SG_ID
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = scene->id;

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SceneCreate* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SceneCreate);

        // initialize memory
        command->type              = SG_COMMAND_SCENE_CREATE;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = scene->id;
    }
    spinlock::unlock(&cq.write_q_lock);

    return scene;
}

void CQ_PushCommand_SceneBGColor(SG_Scene* scene, t_CKVEC4 color)
{
    scene->bg_color = glm::vec4(color.x, color.y, color.z, color.w);

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SceneBGColor* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SceneBGColor);

        // initialize memory
        command->type              = SG_COMMAND_SCENE_BG_COLOR;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = scene->id;
        command->color             = scene->bg_color;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_GeometryCreate(SG_Geometry* geo)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_GeoCreate* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_GeoCreate);

        // initialize memory
        command->type              = SG_COMMAND_GEO_CREATE;
        command->nextCommandOffset = cq.write_q->curr;
        command->geo_type          = geo->geo_type;
        command->sg_id             = geo->id;
        command->params            = geo->params;
    }
    spinlock::unlock(&cq.write_q_lock);
} // Path: src/sg_command.h

// copies data pointer into command arena. does NOT take ownership
void CQ_PushCommand_GeometrySetVertexAttribute(SG_Geometry* geo, int location,
                                               int num_components, f32* data,
                                               int data_len)
{

    BEGIN_COMMAND(SG_Command_GeoSetVertexAttribute,
                  SG_COMMAND_GEO_SET_VERTEX_ATTRIBUTE);

    // get cq memory for vertex data
    f32* attribute_data = ARENA_PUSH_COUNT(cq.write_q, f32, data_len);
    memcpy(attribute_data, data, data_len * sizeof(*data));

    command->sg_id             = geo->id;
    command->num_components    = num_components;
    command->location          = location;
    command->data_len          = data_len;
    command->data_offset       = Arena::offsetOf(cq.write_q, attribute_data);
    command->nextCommandOffset = cq.write_q->curr;

    ASSERT(command->nextCommandOffset - command->data_offset
           == (data_len * sizeof(*data)));

    ASSERT(command->data_len % num_components == 0);

    END_COMMAND();
}

void CQ_PushCommand_GeometrySetIndices(SG_Geometry* geo, u32* indices, int index_count)
{
    BEGIN_COMMAND(SG_Command_GeoSetIndices, SG_COMMAND_GEO_SET_INDICES);

    u32* index_data = ARENA_PUSH_COUNT(cq.write_q, u32, index_count);
    memcpy(index_data, indices, index_count * sizeof(*indices));

    command->sg_id          = geo->id;
    command->index_count    = index_count;
    command->indices_offset = Arena::offsetOf(cq.write_q, index_data);

    END_COMMAND();
}

void CQ_PushCommand_MaterialCreate(SG_Material* material)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_MaterialCreate* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_MaterialCreate);

        // initialize memory
        command->type              = SG_COMMAND_MATERIAL_CREATE;
        command->nextCommandOffset = cq.write_q->curr;
        command->material_type     = material->material_type;
        command->sg_id             = material->id;
        command->params            = material->params;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_Mesh_Create(SG_Mesh* mesh)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_Mesh_Create* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_Mesh_Create);

        // initialize memory
        command->type              = SG_COMMAND_MESH_CREATE;
        command->nextCommandOffset = cq.write_q->curr;
        command->mesh_id           = mesh->id;
        command->geo_id            = mesh->_geo_id;
        command->mat_id            = mesh->_mat_id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_b2World_Set(u32 world_id)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_b2World_Set* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_b2World_Set);

        // initialize memory
        command->type              = SG_COMMAND_b2_WORLD_SET;
        command->nextCommandOffset = cq.write_q->curr;
        command->b2_world_id       = world_id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_b2SubstepCount(u32 substep_count)
{
    BEGIN_COMMAND(SG_Command_b2_SubstepCount, SG_COMMAND_b2_SUBSTEP_COUNT);
    command->substep_count = substep_count;
    END_COMMAND();
}
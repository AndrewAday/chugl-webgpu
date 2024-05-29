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

    Arena* temp = cq.read_q;
    cq.read_q   = cq.write_q;

    // assert read queue has been flushed before swapping
    ASSERT(cq.read_q->curr == 0);

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
    *command
      = (SG_Command*)Arena::get(cq.read_q, (*command)->nextCommandOffset);
    return true;
}

void CQ_ReadCommandQueueClear()
{
    Arena::clear(cq.read_q);
}

// ============================================================================
// Command API
// ============================================================================

void CQ_PushCommand_GG_Scene(SG_Scene* scene)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_GG_Scene* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_GG_Scene);

        // initialize memory
        command->type              = SG_COMMAND_GG_SCENE;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = scene ? scene->id : 0;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_CreateTransform(Chuck_Object* ckobj,
                                    t_CKUINT component_offset_id, CK_DL_API API)
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
        SG_Command_AddChild* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_AddChild);

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
        SG_Command_SetScale* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetScale);

        // initialize memory
        command->type              = SG_COMMAND_SET_SCALE;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->sca               = xform->sca;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SceneCreate(Chuck_Object* ckobj,
                                t_CKUINT component_offset_id, CK_DL_API API)
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
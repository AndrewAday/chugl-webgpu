#include "sg_component.h"
#include "core/hashmap.h"

#include <glm/gtx/quaternion.hpp>

const char* SG_Component::ckname(SG_ComponentType type)
{
    switch (type) {
        case SG_COMPONENT_INVALID: return "Invalid";
        case SG_COMPONENT_TRANSFORM: return "GGen";
        case SG_COMPONENT_GEOMETRY: return "Geometry";
        case SG_COMPONENT_MATERIAL: return "Material";
        case SG_COMPONENT_TEXTURE: return "Texture";
        default: ASSERT(false);
    }
    return "Invalid";
}

// ============================================================================
// SG_Transform definitions
// ============================================================================

void SG_Transform::_init(SG_Transform* t)
{
    ASSERT(t->id == 0); // ensure not initialized twice

    t->pos      = glm::vec3(0.0f);
    t->rot      = QUAT_IDENTITY;
    t->sca      = glm::vec3(1.0f);
    t->parentID = 0;
    // initialize children array for 8 children
    Arena::init(&t->childrenIDs, sizeof(SG_ID) * 8);
}

glm::mat4 SG_Transform::modelMatrix(SG_Transform* t)
{
    return glm::translate(glm::mat4(1.0f), t->pos) * glm::toMat4(t->rot)
           * glm::scale(glm::mat4(1.0f), t->sca);
}

// TODO: can optimize by caching Model and World matrices
glm::mat4 SG_Transform::worldMatrix(SG_Transform* t)
{
    SG_Transform* parent = SG_GetTransform(t->parentID);
    if (!parent) return SG_Transform::modelMatrix(t);

    glm::mat4 worldMat = SG_Transform::modelMatrix(t);
    while (parent) {
        worldMat = SG_Transform::modelMatrix(parent) * worldMat;
        parent   = SG_GetTransform(parent->parentID);
    }
    return worldMat;
}

// walks scene graph, gets world quaternion rotation
glm::quat SG_Transform::worldRotation(SG_Transform* t)
{
    // TODO: is this bugged? does scale affect rotation??
    SG_Transform* parent = SG_GetTransform(t->parentID);
    if (!parent) return t->rot;

    glm::quat worldRot = t->rot;
    while (parent) {
        worldRot = parent->rot * worldRot;
        parent   = SG_GetTransform(parent->parentID);
    }
    return worldRot;
}

glm::vec3 SG_Transform::worldPosition(SG_Transform* t)
{
    SG_Transform* parent = SG_GetTransform(t->parentID);
    if (!parent) return t->pos;
    // multiply by parent's world matrix to get world position
    // DON'T multiply by this world matrix, because it double counts our own
    // transform
    return SG_Transform::worldMatrix(parent) * glm::vec4(t->pos, 1.0);
}

glm::vec3 SG_Transform::worldScale(SG_Transform* t)
{
    glm::vec3 scale      = t->sca;
    SG_Transform* parent = SG_GetTransform(t->parentID);
    while (parent) {
        scale *= parent->sca;
        parent = SG_GetTransform(parent->parentID);
    }
    return scale;
}

void SG_Transform::worldPosition(SG_Transform* t, glm::vec3 pos)
{
    SG_Transform* parent = SG_GetTransform(t->parentID);
    if (!parent)
        t->pos = pos;
    else
        // inverse matrix maps from world space --> local space
        t->pos = glm::inverse(SG_Transform::worldMatrix(parent))
                 * glm::vec4(pos, 1.0);
}

void SG_Transform::worldScale(SG_Transform* t, glm::vec3 scale)
{
    SG_Transform* parent = SG_GetTransform(t->parentID);
    if (!parent)
        t->sca = scale;
    else
        t->sca = scale / SG_Transform::worldScale(parent);
}

#define _SG_XFORM_DIRECTION(t, dir)                                            \
    (glm::normalize(glm::rotate(SG_Transform::worldRotation(t), (dir))))

// get the forward direction in world space
glm::vec3 SG_Transform::forward(SG_Transform* t)
{
    return _SG_XFORM_DIRECTION(t, VEC_FORWARD);
}

// get the right direction in world space
glm::vec3 SG_Transform::right(SG_Transform* t)
{
    return _SG_XFORM_DIRECTION(t, VEC_RIGHT);
}

// get the up direction in world space
glm::vec3 SG_Transform::up(SG_Transform* t)
{
    return _SG_XFORM_DIRECTION(t, VEC_UP);
}
#undef _SG_XFORM_DIRECTION

void SG_Transform::rotateOnLocalAxis(SG_Transform* t, glm::vec3 axis, float deg)
{
    // just flip the order of multiplication to go from local <--> world. so
    // elegant...
    t->rot = t->rot * glm::angleAxis(deg, glm::normalize(axis));
}

void SG_Transform::rotateOnWorldAxis(SG_Transform* t, glm::vec3 axis, float deg)
{
    t->rot = glm::angleAxis(deg, glm::normalize(axis)) * t->rot;
}

void SG_Transform::rotateX(SG_Transform* t, float deg)
{
    SG_Transform::rotateOnLocalAxis(t, VEC_RIGHT, deg);
}

void SG_Transform::rotateY(SG_Transform* t, float deg)
{
    SG_Transform::rotateOnLocalAxis(t, VEC_UP, deg);
}

void SG_Transform::rotateZ(SG_Transform* t, float deg)
{
    SG_Transform::rotateOnLocalAxis(t, VEC_FORWARD, deg);
}

// rotates object to point towards position, updates
void SG_Transform::lookAt(SG_Transform* t, glm::vec3 pos)
{
    SG_Transform* parent = SG_GetTransform(t->parentID);

    glm::quat abs_rotation = glm::conjugate(
      glm::toQuat(glm::lookAt(SG_Transform::worldPosition(t), pos, VEC_UP)));
    glm::quat local_rotation = abs_rotation;

    // convert into relative local rotation based on parent transforms
    if (parent)
        local_rotation
          = glm::inverse(SG_Transform::worldRotation(parent)) * abs_rotation;

    t->rot = local_rotation;
}

// ============================================================================
// SG Component Manager Definitions
// ============================================================================

// storage arenas
static Arena xformArena;
// static Arena geoArena;
// static Arena materialArena;
// static Arena textureArena;

// locators (TODO switch to table)
static hashmap* locator = NULL;

// hashmap item for lookup
struct SG_Location {
    SG_ID id;          // key
    size_t arenaIndex; // value
    Arena* arena;      // where to find
};

static int compareLocation(const void* a, const void* b, void* udata)
{
    SG_Location* locA = (SG_Location*)a;
    SG_Location* locB = (SG_Location*)b;
    return locA->id - locB->id;
}

static u64 hashLocation(const void* item, uint64_t seed0, uint64_t seed1)
{
    // tested xxhash3 is best
    return hashmap_xxhash3(item, sizeof(SG_ID), seed0, seed1);
    // return hashmap_sip(item, sizeof(int), seed0, seed1);
    // return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

// state
static SG_ID nextComponentID = 1; // 0 is reserved for NULL

static SG_ID getNewComponentID()
{
    return nextComponentID++;
}

void SG_Init()
{
    int seed = time(NULL);
    srand(seed);
    locator = hashmap_new(sizeof(int), 0, seed, seed, hashLocation,
                          compareLocation, NULL, NULL);

    Arena::init(&xformArena, sizeof(SG_Transform) * 64);
}

void SG_Free()
{
    // TODO call free() on the components themselves
    Arena::free(&xformArena);

    hashmap_free(locator);
    locator = NULL;
}

SG_Transform* SG_CreateTransform()
{
    size_t index        = ARENA_LENGTH(&xformArena, SG_Transform);
    SG_Transform* xform = ARENA_PUSH_TYPE(&xformArena, SG_Transform);
    SG_Transform::_init(xform);

    xform->id   = getNewComponentID();
    xform->type = SG_COMPONENT_TRANSFORM;

    // store in map
    SG_Location loc = { xform->id, index, &xformArena };
    hashmap_set(locator, &loc);

    return xform;
}

#define _SG_GET_COMPONENT(id)                                                  \
    SG_Location key     = {};                                                  \
    key.id              = id;                                                  \
    SG_Location* result = (SG_Location*)hashmap_get(locator, &key);            \
    SG_Component* component                                                    \
      = result ?                                                               \
          (ARENA_GET_TYPE(result->arena, SG_Transform, result->arenaIndex)) :  \
          NULL;

SG_Component* SG_GetComponent(SG_ID id)
{
    _SG_GET_COMPONENT(id);
    return component;
}

SG_Transform* SG_GetTransform(SG_ID id)
{
    _SG_GET_COMPONENT(id);
    ASSERT(result->arena == &xformArena);
    ASSERT(component->type == SG_COMPONENT_TRANSFORM);
    return (SG_Transform*)component;
}

#undef _SG_GET_COMPONENT
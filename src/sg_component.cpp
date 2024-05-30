#include "sg_component.h"
#include "core/hashmap.h"
#include "geometry.h"

#include <glm/gtx/quaternion.hpp>

// ============================================================================
// SG_Transform definitions
// ============================================================================

void SG_Transform::_init(SG_Transform* t, Chuck_Object* ckobj)
{
    ASSERT(t->id == 0); // ensure not initialized twice
    ASSERT(ckobj);

    t->ckobj    = ckobj;
    t->pos      = glm::vec3(0.0f);
    t->rot      = QUAT_IDENTITY;
    t->sca      = glm::vec3(1.0f);
    t->parentID = 0;
    // initialize children array for 8 children
    Arena::init(&t->childrenIDs, sizeof(SG_ID) * 8);
}

void SG_Transform::translate(SG_Transform* t, glm::vec3 delta)
{
    t->pos += delta;
}

void SG_Transform::rotate(SG_Transform* t, glm::quat q)
{
    t->rot = q * t->rot;
}

void SG_Transform::rotate(SG_Transform* t, glm::vec3 eulers)
{
    t->rot = glm::quat(eulers) * t->rot;
}

void SG_Transform::scale(SG_Transform* t, glm::vec3 s)
{
    t->sca *= s;
}

glm::vec3 SG_Transform::eulerRotationRadians(SG_Transform* t)
{
    return glm::eulerAngles(t->rot);
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

void SG_Transform::rotateOnLocalAxis(SG_Transform* t, glm::vec3 axis, float rad)
{
    // just flip the order of multiplication to go from local <--> world. so
    // elegant...
    t->rot = t->rot * glm::angleAxis(rad, glm::normalize(axis));
}

void SG_Transform::rotateOnWorldAxis(SG_Transform* t, glm::vec3 axis, float rad)
{
    t->rot = glm::angleAxis(rad, glm::normalize(axis)) * t->rot;
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

void SG_Transform::addChild(SG_Transform* parent, SG_Transform* child)
{
    // Object cannot be added as child of itself
    if (parent == child) {
        std::cerr << "GGen cannot be added as child of itself" << std::endl;
        return;
    }

    if (parent == NULL || child == NULL) {
        std::cerr << "cannot add NULL parent or child GGen" << std::endl;
        return;
    }

    if (child->type == SG_COMPONENT_SCENE) { // necessary to prevent cycles
        std::cerr << "cannot add make GScene a child of another GGen"
                  << std::endl;
        return;
    }

    if (SG_Transform::isAncestor(child, parent)) {
        std::cerr
          << "No cycles in scenegraph; cannot add parent as child of descendent"
          << std::endl;
        return;
    }

    // we are already the parent, do nothing
    if (child->parentID == parent->id) return;

    // remove child from old parent
    if (child->parentID) {
        SG_Transform* prevParent = SG_GetTransform(child->parentID);
        SG_Transform::removeChild(prevParent, child);
    }

    // assign to new parent
    child->parentID = parent->id;

    // reference count
    SG_AddRef(parent);

    // add to list of children
    SG_ID* xformID = ARENA_PUSH_TYPE(&parent->childrenIDs, SG_ID);
    *xformID       = child->id;

    // add ref to kid
    SG_AddRef(child);
}
void SG_Transform::removeChild(SG_Transform* parent, SG_Transform* child)
{
    if (child->parentID != parent->id) return;

    size_t numChildren = ARENA_LENGTH(&parent->childrenIDs, SG_ID);
    SG_ID* children    = (SG_ID*)parent->childrenIDs.base;

    child->parentID = 0;

    for (size_t i = 0; i < numChildren; ++i) {
        if (children[i] == child->id) {
            // swap with last element
            children[i] = children[numChildren - 1];
            // pop last element
            Arena::pop(&parent->childrenIDs, sizeof(SG_ID));

            // release ref count on child's chuck object; one less reference to
            // it from us (parent)
            SG_DecrementRef(child->id);

            // release ref count on our (parent's) chuck object; one less
            // reference to it from child
            SG_DecrementRef(parent->id);
            break;
        }
    }
}
bool SG_Transform::isAncestor(SG_Transform* ancestor, SG_Transform* descendent)
{
    while (descendent != NULL) {
        if (descendent == ancestor) return true;
        descendent = SG_GetTransform(descendent->parentID);
    }
    return false;
}

size_t SG_Transform::numChildren(SG_Transform* t)
{
    return ARENA_LENGTH(&t->childrenIDs, SG_ID);
}

SG_Transform* SG_Transform::child(SG_Transform* t, size_t index)
{
    if (index >= numChildren(t)) return NULL;
    return SG_GetTransform(*ARENA_GET_TYPE(&t->childrenIDs, SG_ID, index));
}

// ============================================================================
// SG_Geometry Definitions
// ============================================================================

void SG_Geometry::_init(SG_Geometry* g, SG_GeometryType geo_type, void* params)
{
    ASSERT(params);
    g->geo_type = geo_type;
    switch (geo_type) {
        case SG_GEOMETRY_PLANE: {
            PlaneParams* p  = (PlaneParams*)params;
            g->params.plane = *p;
        } break;
        default: ASSERT(false);
    }
}

// ============================================================================
// SG_Mesh
// ============================================================================

void SG_Mesh::setGeometry(SG_Mesh* mesh, SG_Geometry* geo)
{
    SG_ID oldGeoID = mesh->_geo_id;

    // bump ref on new geometry
    SG_AddRef(geo);

    // assign new geometry
    mesh->_geo_id = geo ? geo->id : 0;

    // decrement ref on old geometry
    SG_DecrementRef(oldGeoID);
}

void SG_Mesh::setMaterial(SG_Mesh* mesh, SG_Material* mat)
{
    SG_ID oldMatID = mesh->_mat_id;

    // bump ref on new material
    SG_AddRef(mat);

    // assign new material
    mesh->_mat_id = mat ? mat->id : 0;

    // decrement ref on old material
    SG_DecrementRef(oldMatID);
}

// ============================================================================
// SG Component Manager Definitions
// ============================================================================

// chugin API pointers
static const Chuck_DL_Api* _ck_api = NULL;

// GC arenas (TODO move into struct)
static Arena _gc_queue_a;
static Arena _gc_queue_b;
static Arena* _gc_queue_read  = &_gc_queue_a;
static Arena* _gc_queue_write = &_gc_queue_b;

// storage arenas
static Arena SG_XformArena;
static Arena SG_SceneArena;
static Arena SG_GeoArena;
static Arena SG_MaterialArena;
static Arena SG_MeshArena;
// static Arena textureArena;

// locators (TODO switch to table)
static hashmap* locator = NULL;

// hashmap item for lookup
struct SG_Location {
    SG_ID id;      // key
    size_t offset; // value (byte offset into arena)
    Arena* arena;  // where to find
};

static int _SG_CompareLocation(const void* a, const void* b, void* udata)
{
    SG_Location* locA = (SG_Location*)a;
    SG_Location* locB = (SG_Location*)b;
    return locA->id - locB->id;
}

static u64 _SG_HashLocation(const void* item, uint64_t seed0, uint64_t seed1)
{
    // tested xxhash3 is best
    SG_Location* key = (SG_Location*)item;
    return hashmap_xxhash3(&key->id, sizeof(SG_ID), seed0, seed1);
    // return hashmap_sip(item, sizeof(int), seed0, seed1);
    // return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

// state
static SG_ID SG_NextComponentID = 1; // 0 is reserved for NULL

static SG_ID SG_GetNewComponentID()
{
    return SG_NextComponentID++;
}

void SG_Init(const Chuck_DL_Api* api)
{
    _ck_api = api;

    int seed = time(NULL);
    srand(seed);
    locator = hashmap_new(sizeof(SG_Location), 0, seed, seed, _SG_HashLocation,
                          _SG_CompareLocation, NULL, NULL);

    Arena::init(&SG_XformArena, sizeof(SG_Transform) * 64);
    Arena::init(&SG_SceneArena, sizeof(SG_Scene) * 64);
    Arena::init(&SG_GeoArena, sizeof(SG_Geometry) * 32);
    Arena::init(&SG_MaterialArena, sizeof(SG_Material) * 32);
    Arena::init(&SG_MeshArena, sizeof(SG_Mesh) * 64);

    // init gc state
    Arena::init(&_gc_queue_a, sizeof(SG_ID) * 64);
    Arena::init(&_gc_queue_b, sizeof(SG_ID) * 64);
}

void SG_Free()
{
    _ck_api = NULL;

    // TODO call free() on the components themselves
    Arena::free(&SG_XformArena);
    Arena::free(&SG_SceneArena);
    Arena::free(&SG_GeoArena);
    Arena::free(&SG_MaterialArena);
    Arena::free(&SG_MeshArena);

    hashmap_free(locator);
    locator = NULL;

    // free gc state
    Arena::free(&_gc_queue_a);
    Arena::free(&_gc_queue_b);
}

SG_Transform* SG_CreateTransform(Chuck_Object* ckobj)
{
    size_t offset       = SG_XformArena.curr;
    SG_Transform* xform = ARENA_PUSH_TYPE(&SG_XformArena, SG_Transform);
    SG_Transform::_init(xform, ckobj);

    xform->id   = SG_GetNewComponentID();
    xform->type = SG_COMPONENT_TRANSFORM;

    // store in map
    SG_Location loc = { xform->id, offset, &SG_XformArena };
    hashmap_set(locator, &loc);

    return xform;
}

SG_Scene* SG_CreateScene(Chuck_Object* ckobj)
{
    Arena* arena    = &SG_SceneArena;
    size_t offset   = arena->curr;
    SG_Scene* scene = ARENA_PUSH_TYPE(arena, SG_Scene);

    SG_Scene::_init(scene, ckobj);

    scene->id   = SG_GetNewComponentID();
    scene->type = SG_COMPONENT_SCENE;

    // store in map
    SG_Location loc = { scene->id, offset, arena };
    hashmap_set(locator, &loc);

    return scene;
}

SG_Geometry* SG_CreateGeometry(Chuck_Object* ckobj, SG_GeometryType geo_type,
                               void* params)
{
    Arena* arena     = &SG_GeoArena;
    size_t offset    = arena->curr;
    SG_Geometry* geo = ARENA_PUSH_TYPE(arena, SG_Geometry);

    SG_Geometry::_init(geo, geo_type, params);

    geo->id    = SG_GetNewComponentID();
    geo->type  = SG_COMPONENT_GEOMETRY;
    geo->ckobj = ckobj;

    // store in map
    SG_Location loc = { geo->id, offset, arena };
    hashmap_set(locator, &loc);

    return geo;
}

SG_Material* SG_CreateMaterial(Chuck_Object* ckobj,
                               SG_MaterialType material_type, void* params)
{
    Arena* arena     = &SG_MaterialArena;
    size_t offset    = arena->curr;
    SG_Material* mat = ARENA_PUSH_TYPE(arena, SG_Material);

    mat->ckobj         = ckobj;
    mat->id            = SG_GetNewComponentID();
    mat->type          = SG_COMPONENT_MATERIAL;
    mat->material_type = material_type;

    switch (material_type) {
        case SG_MATERIAL_PBR: {
            if (params == NULL) {
                // copy default values
                SG_Material_PBR_Params p = {};
                COPY_STRUCT(&mat->params.pbr, &p, SG_Material_PBR_Params);
                // confirm default values
                ASSERT(mat->params.pbr.baseColor == glm::vec4(1.0f));
            } else {
                COPY_STRUCT(&mat->params.pbr, (SG_Material_PBR_Params*)params,
                            SG_Material_PBR_Params);
            }
        } break;
        default: ASSERT(false);
    }

    // store in map
    SG_Location loc = { mat->id, offset, arena };
    hashmap_set(locator, &loc);

    return mat;
}

SG_Mesh* SG_CreateMesh(Chuck_Object* ckobj, SG_Geometry* sg_geo,
                       SG_Material* sg_mat)
{
    Arena* arena  = &SG_MeshArena;
    size_t offset = arena->curr;
    SG_Mesh* mesh = ARENA_PUSH_TYPE(arena, SG_Mesh);
    SG_Transform::_init(mesh, ckobj); // init transform data

    mesh->ckobj = ckobj;
    mesh->id    = SG_GetNewComponentID();
    mesh->type  = SG_COMPONENT_MESH;
    SG_Mesh::setGeometry(mesh, sg_geo);
    SG_Mesh::setMaterial(mesh, sg_mat);

    // store in map
    SG_Location loc = { mesh->id, offset, arena };
    hashmap_set(locator, &loc);

    return mesh;
}

SG_Component* SG_GetComponent(SG_ID id)
{
    SG_Location key     = {};
    key.id              = id;
    SG_Location* result = (SG_Location*)hashmap_get(locator, &key);
    SG_Component* component
      = result ? (SG_Component*)Arena::get(result->arena, result->offset) :
                 NULL;
    return component;
}

SG_Transform* SG_GetTransform(SG_ID id)
{
    SG_Component* component = SG_GetComponent(id);
    ASSERT(component == NULL || component->type == SG_COMPONENT_TRANSFORM
           || component->type == SG_COMPONENT_SCENE);
    // TODO: also check for other children of SG_Transform
    return (SG_Transform*)component;
}

SG_Scene* SG_GetScene(SG_ID id)
{
    SG_Component* component = SG_GetComponent(id);
    ASSERT(component->type == SG_COMPONENT_SCENE);
    return (SG_Scene*)component;
}

SG_Geometry* SG_GetGeometry(SG_ID id)
{
    SG_Component* component = SG_GetComponent(id);
    ASSERT(component->type == SG_COMPONENT_GEOMETRY);
    return (SG_Geometry*)component;
}

SG_Material* SG_GetMaterial(SG_ID id)
{
    SG_Component* component = SG_GetComponent(id);
    ASSERT(component->type == SG_COMPONENT_MATERIAL);
    return (SG_Material*)component;
}

SG_Mesh* SG_GetMesh(SG_ID id)
{
    SG_Component* component = SG_GetComponent(id);
    ASSERT(component->type == SG_COMPONENT_MESH);
    return (SG_Mesh*)component;
}

// ============================================================================
// SG Garbage Collector
// ============================================================================

static void _SG_SwapGCQueues()
{
    Arena* temp     = _gc_queue_read;
    _gc_queue_read  = _gc_queue_write;
    _gc_queue_write = temp;
}

void SG_DecrementRef(SG_ID id)
{
    if (id == 0) return; // NULL component
    SG_ID* idPtr = ARENA_PUSH_TYPE(_gc_queue_write, SG_ID);
    *idPtr       = id;
}

void SG_AddRef(SG_Component* comp)
{
    if (comp == NULL) return;
    ASSERT(comp->ckobj);
    _ck_api->object->add_ref(comp->ckobj);
}

void SG_GC()
{
    // swap queues
    _SG_SwapGCQueues();

    // release all objects in read queue
    size_t count = ARENA_LENGTH(_gc_queue_read, SG_ID);
    for (size_t i = 0; i < count; i++) {
        SG_Component* comp = ARENA_GET_TYPE(_gc_queue_read, SG_Component, i);
        if (!comp) continue; // already deleted
        _ck_api->object->release(comp->ckobj);
    }

    // clear read queue
    Arena::clear(_gc_queue_read);
}
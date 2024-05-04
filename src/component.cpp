#include "component.h"
#include "geometry.h"
#include "graphics.h"
#include "shaders.h"

#include "core/log.h"

#include <unordered_map>

/*
XForm system:
- each XForm component is marked with a stale flag NONE/DESCENDENTS/WORLD/LOCAL
- when an XForm is reparented or moved, it marks itself as stale WORLD/LOCAL and
all its parents as DESCENDENTS
    - DESCENDENTS means my own transform is fine, but at least 1 descendent
needs to be recomputed
    - WORLD means my local matrix is fine, but my world matrix needs to be
recomputed
    - LOCAL means both local and world matrices need to be recomputed
- at the start of each render, after all updates, the graphics thread needs to
call SG_Transform::rebuildMatrices(root) to update all world matrices
    - the staleness flags optimizes this process by ignoring all branches that
don't require updating
    - during this rebuild, transforms that have been marked as WORLD or LOCAL
will also mark their geometry as stale

Geometry system:
- all draws are instanced. All xforms bound to a certain geometry and material
will be rendered in a single draw call
- each geometry component has a storage buffer that holds all the world matrices
of its associated xform instances
- if a xform is moved or reparented, the geometry component will be marked
stale, and the storage buffer will be rebuilt via SG_Geometry::rebuildBindGroup

Component Manager:
- handles all creation and deletion of components
- components (Xforms, Geos ...) are stored in contiguous Arena memory
- deletions are handled by swapping the deleted component with the last
component
- each component has a unique ID
- a hashmap stores the ID to offset mapping
  - store offset and not pointer because pointers can be invalidated if the
arena grows, or elements within are deleted and swapped

*/

// ============================================================================
// Forward Declarations
// ============================================================================

static SG_ID getNewComponentID();

// ============================================================================
// Transform Component
// ============================================================================

void SG_Transform::init(SG_Transform* transform)
{
    ASSERT(transform->id == 0); // ensure not initialized twice
    *transform = {};

    transform->id   = getNewComponentID();
    transform->type = SG_COMPONENT_TRANSFORM;

    transform->_pos = glm::vec3(0.0f);
    transform->_rot = QUAT_IDENTITY;
    transform->_sca = glm::vec3(1.0f);

    transform->world  = MAT_IDENTITY;
    transform->local  = MAT_IDENTITY;
    transform->_stale = SG_TRANSFORM_STALE_NONE;

    transform->parentID = 0;
    // initialize children array for 8 children
    Arena::init(&transform->children, sizeof(SG_ID) * 8);
}

void SG_Transform::setStale(SG_Transform* xform, SG_Transform_Staleness stale)
{
    // only set if new staleness is higher priority
    if (stale > xform->_stale) xform->_stale = stale;

    // propagate staleness to parent
    // it is assumed that if a parent has staleness, all its parents will
    // also have been marked with prio at least SG_TRANSFORM_STALE_DESCENDENTS
    while (xform->parentID != 0) {
        SG_Transform* parent = Component_GetXform(xform->parentID);
        // upwards stale chain already established. skip
        if (parent->_stale > SG_TRANSFORM_STALE_NONE) break;
        // otherwise set parent staleness and continue propagating
        parent->_stale = SG_TRANSFORM_STALE_DESCENDENTS;
        xform          = parent;
    }
}

bool SG_Transform::isAncestor(SG_Transform* ancestor, SG_Transform* descendent)
{
    while (descendent != NULL) {
        if (descendent == ancestor) return true;
        descendent = Component_GetXform(descendent->parentID);
    }
    return false;
}

void SG_Transform::removeChild(SG_Transform* parent, SG_Transform* child)
{
    if (child->parentID != parent->id) {
        log_error("cannot remove a child who does not belong to parent");
        return;
    }

    u32 numChildren = ARENA_LENGTH(&parent->children, SG_ID);
    u32* children   = (u32*)parent->children.base;

    // remove child's parent reference
    // (must do this first to prevent double-free, bc
    // releasing the child may trigger a disconnect its the destructor)
    child->parentID = 0;

    // remove from parent
    for (u32 i = 0; i < numChildren; ++i) {
        if (children[i] == child->id) {
            // swap with last element
            children[i] = children[numChildren - 1];
            // pop last element
            Arena::pop(&parent->children, sizeof(SG_ID));
            break;
        }
    }

    // TODO: gc
    // release ref count on child's chuck object; one less reference to it
    // from us (parent)
    // CHUGL_NODE_QUEUE_RELEASE(child);

    // release ref count on our (parent's) chuck object; one less reference
    // to it from child
    // CHUGL_NODE_QUEUE_RELEASE(parent);
}

void SG_Transform::addChild(SG_Transform* parent, SG_Transform* child)
{
    ASSERT(!SG_Transform::isAncestor(child, parent)); // prevent cycles
    ASSERT(parent != NULL);
    ASSERT(child != NULL);

    if (SG_Transform::isAncestor(child, parent)) {
        log_error(
          "No cycles in scenegraph; cannot add parent as child of descendent");
        return;
    }

    if (parent == NULL || child == NULL) {
        log_error("Cannot add NULL parent or child to scenegraph");
        return;
    }

    // relationship already in place, do noting
    if (child->parentID == parent->id) return;

    // remove child from previous parent
    if (child->parentID != 0) {
        SG_Transform* prevParent = Component_GetXform(child->parentID);
        SG_Transform::removeChild(prevParent, child);
    }

    // set parent of child
    child->parentID = parent->id;
    // TODO: reference count
    // CHUGL_NODE_ADD_REF(this);

    // add child to parent
    SG_ID* xformID = ARENA_PUSH_TYPE(&parent->children, SG_ID);
    *xformID       = child->id;

    // TODO: refcount
    // add ref to kid
    // CHUGL_NODE_ADD_REF(child);

    SG_Transform::setStale(child, SG_TRANSFORM_STALE_WORLD);
}

glm::mat4 SG_Transform::localMatrix(SG_Transform* xform)
{
    glm::mat4 M = glm::mat4(1.0);
    M           = glm::translate(M, xform->_pos);
    M           = M * glm::toMat4(xform->_rot);
    M           = glm::scale(M, xform->_sca);
    return M;
}

/// @brief decompose matrix into transform data
void SG_Transform::setXformFromMatrix(SG_Transform* xform, const glm::mat4& M)
{
    log_trace("decomposing matrix");
    xform->_pos = glm::vec3(M[3]);
    xform->_rot = glm::quat_cast(M);
    xform->_sca
      = glm::vec3(glm::length(M[0]), glm::length(M[1]), glm::length(M[2]));
    xform->local = M;

    // log_trace("pos: %s", glm::to_string(xform->_pos).c_str());
    // log_trace("rot: %s", glm::to_string(xform->_rot).c_str());
    // log_trace("sca: %s", glm::to_string(xform->_sca).c_str());

    SG_Transform::setStale(xform, SG_TRANSFORM_STALE_LOCAL);
}

void SG_Transform::setXform(SG_Transform* xform, const glm::vec3& pos,
                            const glm::quat& rot, const glm::vec3& sca)
{
    xform->_pos = pos;
    xform->_rot = rot;
    xform->_sca = sca;
    SG_Transform::setStale(xform, SG_TRANSFORM_STALE_LOCAL);
}

void SG_Transform::pos(SG_Transform* xform, const glm::vec3& pos)
{
    xform->_pos = pos;
    SG_Transform::setStale(xform, SG_TRANSFORM_STALE_LOCAL);
}

void SG_Transform::rot(SG_Transform* xform, const glm::quat& rot)
{
    xform->_rot = rot;
    SG_Transform::setStale(xform, SG_TRANSFORM_STALE_LOCAL);
}

void SG_Transform::sca(SG_Transform* xform, const glm::vec3& sca)
{
    xform->_sca = sca;
    SG_Transform::setStale(xform, SG_TRANSFORM_STALE_LOCAL);
}

/// @brief recursive helper to regen matrices for xform and all descendents
static void _Transform_RebuildDescendants(SG_Transform* xform,
                                          const glm::mat4* parentWorld)
{
    // mark geo as stale since world matrix will change
    if (xform->_geoID != 0) {
        SG_Geometry* geo = Component_GetGeo(xform->_geoID);
        ASSERT(geo != NULL);
        geo->staleBindGroup = true;
    }

    // rebuild local mat
    if (xform->_stale == SG_TRANSFORM_STALE_LOCAL)
        xform->local = SG_Transform::localMatrix(xform);

    // always rebuild world mat
    xform->world = (*parentWorld) * xform->local;

    // set fresh
    xform->_stale = SG_TRANSFORM_STALE_NONE;

    // rebuild all children
    SG_ID* children = (SG_ID*)xform->children.base;
    for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
        SG_Transform* child = Component_GetXform(children[i]);
        ASSERT(child != NULL);
        _Transform_RebuildDescendants(child, &xform->world);
    }
}

void SG_Transform::rebuildMatrices(SG_Transform* root, Arena* arena)
{
    // push root onto stack
    SG_ID* base = ARENA_PUSH_TYPE(arena, SG_ID);
    *base       = root->id;
    SG_ID* top  = (SG_ID*)Arena::top(arena);

    ASSERT(base + 1 == top);

    glm::mat4 identityMat = MAT_IDENTITY;

    // while stack is not empty
    while (top != base) {
        // pop id from stack
        SG_ID xformID = top[-1];
        Arena::pop(arena, sizeof(SG_ID));
        SG_Transform* xform = Component_GetXform(xformID);
        ASSERT(xform != NULL);

        switch (xform->_stale) {
            case SG_TRANSFORM_STALE_NONE: break;
            case SG_TRANSFORM_STALE_DESCENDENTS: {
                // add to stack
                SG_ID* children = (SG_ID*)xform->children.base;
                for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID);
                     ++i) {
                    SG_ID* childID = ARENA_PUSH_TYPE(arena, SG_ID);
                    *childID       = children[i];
                }
                break;
            }
            case SG_TRANSFORM_STALE_WORLD:
            case SG_TRANSFORM_STALE_LOCAL: {
                // get parent world matrix
                SG_Transform* parent = Component_GetXform(xform->parentID);
                _Transform_RebuildDescendants(xform, parent ? &parent->world :
                                                              &identityMat);
                break;
            }
            default: log_error("unhandled staleness %d", xform->_stale); break;
        }

        // always set fresh
        xform->_stale = SG_TRANSFORM_STALE_NONE;

        // update stack top
        top = (SG_ID*)Arena::top(arena);
    }
}

u32 SG_Transform::numChildren(SG_Transform* xform)
{
    return ARENA_LENGTH(&xform->children, SG_ID);
}

SG_Transform* SG_Transform::getChild(SG_Transform* xform, u32 index)
{
    SG_ID* children     = (SG_ID*)xform->children.base;
    SG_Transform* child = Component_GetXform(children[index]);
    return child;
}

void SG_Transform::rotateOnLocalAxis(SG_Transform* xform, glm::vec3 axis,
                                     f32 deg)
{
    SG_Transform::rot(xform,
                      xform->_rot * glm::angleAxis(deg, glm::normalize(axis)));
}

void SG_Transform::rotateOnWorldAxis(SG_Transform* xform, glm::vec3 axis,
                                     f32 deg)
{
    SG_Transform::rot(xform,
                      glm::angleAxis(deg, glm::normalize(axis)) * xform->_rot);
}

void SG_Transform::print(SG_Transform* xform, u32 depth)
{
    for (u32 i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("%s\n", xform->name.c_str());
    u32 childrenCount = SG_Transform::numChildren(xform);
    for (u32 i = 0; i < childrenCount; ++i) {
        print(SG_Transform::getChild(xform, i), depth + 1);
    }
}

// ============================================================================
// Geometry Component
// ============================================================================
void SG_Geometry::init(SG_Geometry* geo)
{
    ASSERT(geo->id == 0);
    *geo = {};

    geo->id   = getNewComponentID();
    geo->type = SG_COMPONENT_GEOMETRY;
    Arena::init(&geo->xformIDs, sizeof(SG_ID) * 8);
}

void SG_Geometry::buildFromVertices(GraphicsContext* gctx, SG_Geometry* geo,
                                    Vertices* vertices)
{
    IndexBuffer::init(gctx, &geo->indexBuffer, vertices->indicesCount,
                      vertices->indices, NULL);
    VertexBuffer::init(gctx, &geo->vertexBuffer, vertices->vertexCount,
                       vertices->vertexData, NULL);
    geo->numVertices = vertices->vertexCount;
    geo->numIndices  = vertices->indicesCount;
    log_trace("geo->numVertices: %d", geo->numVertices);
    log_trace("geo->numIndices: %d", geo->numIndices);
}
u64 SG_Geometry::numInstances(SG_Geometry* geo)
{
    return ARENA_LENGTH(&geo->xformIDs, u64);
}

void SG_Geometry::addXform(SG_Geometry* geo, SG_Transform* xform)
{
// check we aren't inserting a duplicate
#ifdef CHUGL_DEBUG
    for (u32 i = 0; i < ARENA_LENGTH(&geo->xformIDs, u64); ++i) {
        if (*ARENA_GET_TYPE(&geo->xformIDs, SG_ID, i) == xform->id) {
            log_error("xform already exists in geometry");
            return;
        }
    }
#endif

    // set stale
    geo->staleBindGroup = true;

    // remove xform from previous geometry
    if (xform->_geoID != 0) {
        SG_Geometry* prevGeo = Component_GetGeo(xform->_geoID);
        ASSERT(
          prevGeo
          != NULL); // reference counting should prevent geo from being deleted
        SG_Geometry::removeXform(prevGeo, xform);
    }

    // add xform to new geometry
    xform->_geoID = geo->id;
    u64* xformID  = ARENA_PUSH_TYPE(&geo->xformIDs, u64);
    *xformID      = xform->id;

    // TODO refcounting
}

void SG_Geometry::removeXform(SG_Geometry* geo, SG_Transform* xform)
{
    u64 numInstances = SG_Geometry::numInstances(geo);
    SG_ID* instances = (SG_ID*)geo->xformIDs.base;
    for (u32 i = 0; i < numInstances; ++i) {
        u64 id = instances[i];
        if (id == xform->id) {
            // swap with last element
            instances[i] = instances[numInstances - 1];
            // pop last element
            Arena::popZero(&geo->xformIDs, sizeof(u64));
            // set stale
            geo->staleBindGroup = true;
            break;
        }
    }
}

// recreates storagebuffer based on #xformIDs and rebuilds bindgroup
// populates storage buffer with new xform data
// only creates new storage buffer if #xformIDs grows or an existing xformID is
// moved
void SG_Geometry::rebuildBindGroup(GraphicsContext* gctx, SG_Geometry* geo,
                                   WGPUBindGroupLayout layout, Arena* arena)
{
    if (!geo->staleBindGroup) return;

    log_trace("rebuilding bindgroup for geometry %d", geo->id);
    geo->staleBindGroup = false;

    // build new array of matrices on CPU
    glm::mat4* modelMatrices = (glm::mat4*)Arena::top(arena);
    u32 matrixCount          = 0;

    u64 numInstances = SG_Geometry::numInstances(geo);
    SG_ID* xformIDs  = (SG_ID*)geo->xformIDs.base;
    // delete and swap any destroyed xforms
    for (u32 i = 0; i < numInstances; ++i) {
        SG_Transform* xform = Component_GetXform(xformIDs[i]);
        if (xform == NULL) {
            // swap with last element
            xformIDs[i] = xformIDs[numInstances - 1];
            // pop last element
            Arena::pop(&geo->xformIDs, sizeof(SG_ID));
            // decrement to reprocess this index
            --i;
            --numInstances;
            continue;
        }
        // assert his xform belongs to this geo
        ASSERT(xform->_geoID == geo->id);
        // else add xform matrix to arena
        ++matrixCount;
        // world matrix should already have been computed by now
        ASSERT(xform->_stale == SG_TRANSFORM_STALE_NONE);
        glm::mat4* matPtr = ARENA_PUSH_TYPE(arena, glm::mat4);
        *matPtr           = xform->world;
    }
    // sanity check that we have the correct number of matrices
    ASSERT(matrixCount == SG_Geometry::numInstances(geo));
    // update numInstances after possibly deleting some
    numInstances = matrixCount;

    // rebuild storage buffer only if it needs to grow
    if (numInstances > geo->storageBufferCap) {
        // double capacity (like dynamic array)
        u64 newCap = MAX(numInstances, geo->storageBufferCap * 2);
        log_trace(
          "growing storage buffer for geometry %d from capacity: %d to "
          "%d",
          geo->id, geo->storageBufferCap, newCap);

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size                 = sizeof(DrawUniforms) * newCap;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
        // destroy previous
        if (geo->storageBuffer != NULL) {
            wgpuBufferDestroy(geo->storageBuffer);
            wgpuBufferRelease(geo->storageBuffer);
        }
        geo->storageBuffer = wgpuDeviceCreateBuffer(gctx->device, &bufferDesc);
        // update new capacity
        geo->storageBufferCap = newCap;
    }

    // populate storage buffer with new xform data
    wgpuQueueWriteBuffer(gctx->queue, geo->storageBuffer, 0, modelMatrices,
                         matrixCount * sizeof(modelMatrices[0]));

    // pop arena after copying data to GPU
    ARENA_POP_COUNT(arena, glm::mat4, matrixCount);
    ASSERT(modelMatrices
           == Arena::top(arena)); // make sure arena is back to original state

    // create new bindgroup for new storage buffer
    {
        geo->bindGroupEntry         = {};
        geo->bindGroupEntry.binding = 0; // @binding(0)
        geo->bindGroupEntry.offset  = 0;
        geo->bindGroupEntry.buffer
          = geo->storageBuffer; // only supports uniform buffers for now
        // note: bind size is NOT same as buffer capacity
        u64 bindingSize          = numInstances * sizeof(DrawUniforms);
        geo->bindGroupEntry.size = bindingSize;

        // release previous
        if (geo->bindGroup) wgpuBindGroupRelease(geo->bindGroup);

        // A bind group contains one or multiple bindings
        WGPUBindGroupDescriptor desc = {};
        desc.layout                  = layout;
        desc.entries                 = &geo->bindGroupEntry;
        desc.entryCount              = 1; // force 1 binding per group
        geo->bindGroup = wgpuDeviceCreateBindGroup(gctx->device, &desc);
    }
}

// ============================================================================
// Component Manager Definitions
// ============================================================================

// storage arenas
static Arena xformArena;
static Arena geoArena;

// maps from id --> offset
// TODO: switch to better hashmap impl
static std::unordered_map<SG_ID, u64> xformMap;
static std::unordered_map<SG_ID, u64> geoMap;

static SG_ID _componentIDCounter = 1; // reserve 0 for NULL

void Component_Init()
{
    // initialize arena memory
    Arena::init(&xformArena, sizeof(SG_Transform) * 128);
    Arena::init(&geoArena, sizeof(SG_Geometry) * 128);
}

void Component_Free()
{
    // free arena memory
    Arena::free(&xformArena);
    Arena::free(&geoArena);
}

static SG_ID getNewComponentID()
{
    return _componentIDCounter++;
}

SG_Transform* Component_CreateTransform()
{
    SG_Transform* xform = ARENA_PUSH_TYPE(&xformArena, SG_Transform);
    SG_Transform::init(xform);

    // store offset
    xformMap[xform->id] = (u64)((u8*)xform - xformArena.base);

    return xform;
}

SG_Geometry* Component_CreateGeometry()
{
    SG_Geometry* geo = ARENA_PUSH_TYPE(&geoArena, SG_Geometry);
    SG_Geometry::init(geo);

    // store offset
    geoMap[geo->id] = (u64)((u8*)geo - geoArena.base);

    return geo;
}

SG_Transform* Component_GetXform(u64 id)
{
    auto it = xformMap.find(id);
    if (it == xformMap.end()) return NULL;

    return (SG_Transform*)Arena::get(&xformArena, it->second);
}

SG_Geometry* Component_GetGeo(u64 id)
{
    auto it = geoMap.find(id);
    if (it == geoMap.end()) return NULL;

    return (SG_Geometry*)Arena::get(&geoArena, it->second);
}
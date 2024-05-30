#include "r_component.h"
#include "core/hashmap.h"
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
call R_Transform::rebuildMatrices(root) to update all world matrices
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
stale, and the storage buffer will be rebuilt via R_Geometry::rebuildBindGroup

Component Manager:
- handles all creation and deletion of components
- components (Xforms, Geos ...) are stored in contiguous Arena memory
- deletions are handled by swapping the deleted component with the last
component
- each component has a unique ID
- a hashmap stores the ID to offset mapping
  - store offset and not pointer because pointers can be invalidated if the
arena grows, or elements within are deleted and swapped

Note: the stale flag scenegraph system allows for a nice programming pattern
in ChucK:

// put all objects that can move under here
GGen dynamic --> GG.scene();
// put all immobile objects here
GGen static --> GG.scene();

all static GGens are grouped under a single XForm, and this entire branch of the
scenegraph will be skipped every frame!

*/

// ============================================================================
// Forward Declarations
// ============================================================================

static SG_ID _componentIDCounter = 1; // reserve 0 for NULL

// All R_IDs are negative to avoid conflict with positive SG_IDs
static R_ID _R_IDCounter = -1; // reserve 0 for NULL.

static SG_ID getNewComponentID()
{
    return _componentIDCounter++;
}

static R_ID getNewRID()
{
    return _R_IDCounter--;
}

// ============================================================================
// Transform Component
// ============================================================================

void R_Transform::init(R_Transform* transform)
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
    transform->normal = MAT_IDENTITY;
    transform->_stale = R_Transform_STALE_NONE;

    transform->parentID = 0;
    // initialize children array for 8 children
    Arena::init(&transform->children, sizeof(SG_ID) * 8);
}

void R_Transform::initFromSG(R_Transform* r_xform, SG_Command_CreateXform* cmd)
{
    ASSERT(r_xform->id == 0); // ensure not initialized twice
    *r_xform = {};

    // copy base component data
    // TODO have a separate R_ComponentType enum?
    r_xform->id   = cmd->sg_id;
    r_xform->type = SG_COMPONENT_TRANSFORM;

    // copy xform
    r_xform->_pos   = cmd->pos;
    r_xform->_rot   = cmd->rot;
    r_xform->_sca   = cmd->sca;
    r_xform->_stale = R_Transform_STALE_LOCAL;

    // initialize children array for 8 children
    Arena::init(&r_xform->children, sizeof(SG_ID) * 8);
}

void R_Transform::setStale(R_Transform* xform, R_Transform_Staleness stale)
{
    // only set if new staleness is higher priority
    if (stale > xform->_stale) xform->_stale = stale;

    // propagate staleness to parent
    // it is assumed that if a parent has staleness, all its parents will
    // also have been marked with prio at least R_Transform_STALE_DESCENDENTS
    while (xform->parentID != 0) {
        R_Transform* parent = Component_GetXform(xform->parentID);
        // upwards stale chain already established. skip
        if (parent->_stale > R_Transform_STALE_NONE) break;
        // otherwise set parent staleness and continue propagating
        parent->_stale = R_Transform_STALE_DESCENDENTS;
        xform          = parent;
    }
}

bool R_Transform::isAncestor(R_Transform* ancestor, R_Transform* descendent)
{
    while (descendent != NULL) {
        if (descendent == ancestor) return true;
        descendent = Component_GetXform(descendent->parentID);
    }
    return false;
}

void R_Transform::removeChild(R_Transform* parent, R_Transform* child)
{
    if (child->parentID != parent->id) {
        log_error("cannot remove a child who does not belong to parent");
        return;
    }

    size_t numChildren = ARENA_LENGTH(&parent->children, SG_ID);
    SG_ID* children    = (SG_ID*)parent->children.base;

    // remove child's parent reference
    child->parentID = 0;

    // remove from parent
    for (size_t i = 0; i < numChildren; ++i) {
        if (children[i] == child->id) {
            // swap with last element
            children[i] = children[numChildren - 1];
            // pop last element
            Arena::pop(&parent->children, sizeof(SG_ID));
            break;
        }
    }
}

void R_Transform::addChild(R_Transform* parent, R_Transform* child)
{
    ASSERT(!R_Transform::isAncestor(child, parent)); // prevent cycles
    ASSERT(parent != NULL);
    ASSERT(child != NULL);

    if (R_Transform::isAncestor(child, parent)) {
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
        R_Transform* prevParent = Component_GetXform(child->parentID);
        R_Transform::removeChild(prevParent, child);
    }

    // set parent of child
    child->parentID = parent->id;

    // add child to parent
    SG_ID* xformID = ARENA_PUSH_TYPE(&parent->children, SG_ID);
    *xformID       = child->id;

    R_Transform::setStale(child, R_Transform_STALE_WORLD);
}

glm::mat4 R_Transform::localMatrix(R_Transform* xform)
{
    glm::mat4 M = glm::mat4(1.0);
    M           = glm::translate(M, xform->_pos);
    M           = M * glm::toMat4(xform->_rot);
    M           = glm::scale(M, xform->_sca);
    return M;
}

/// @brief decompose matrix into transform data
void R_Transform::setXformFromMatrix(R_Transform* xform, const glm::mat4& M)
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

    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::setXform(R_Transform* xform, const glm::vec3& pos,
                           const glm::quat& rot, const glm::vec3& sca)
{
    xform->_pos = pos;
    xform->_rot = rot;
    xform->_sca = sca;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::pos(R_Transform* xform, const glm::vec3& pos)
{
    xform->_pos = pos;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::rot(R_Transform* xform, const glm::quat& rot)
{
    xform->_rot = rot;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::sca(R_Transform* xform, const glm::vec3& sca)
{
    xform->_sca = sca;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

/// @brief recursive helper to regen matrices for xform and all descendents
static void _Transform_RebuildDescendants(R_Transform* xform,
                                          const glm::mat4* parentWorld)
{
    // mark primitive as stale since world matrix will change
    if (xform->_geoID && xform->_matID) {
        R_Material* mat = Component_GetMaterial(xform->_matID);
        ASSERT(mat != NULL);
        R_Material::markPrimitiveStale(mat, xform);
    }

    // rebuild local mat
    if (xform->_stale == R_Transform_STALE_LOCAL)
        xform->local = R_Transform::localMatrix(xform);

    // always rebuild world mat
    xform->world = (*parentWorld) * xform->local;

    // rebuild normal matrix
    xform->normal = glm::transpose(glm::inverse(xform->world));

    // set fresh
    xform->_stale = R_Transform_STALE_NONE;

    // rebuild all children
    for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
        R_Transform* child
          = Component_GetXform(*ARENA_GET_TYPE(&xform->children, SG_ID, i));
        ASSERT(child != NULL);
        _Transform_RebuildDescendants(child, &xform->world);
    }
}

void R_Transform::rebuildMatrices(R_Transform* root, Arena* arena)
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
        R_Transform* xform = Component_GetXform(xformID);
        ASSERT(xform != NULL);

        switch (xform->_stale) {
            case R_Transform_STALE_NONE: break;
            case R_Transform_STALE_DESCENDENTS: {
                // add to stack
                SG_ID* children = (SG_ID*)xform->children.base;
                for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID);
                     ++i) {
                    SG_ID* childID = ARENA_PUSH_TYPE(arena, SG_ID);
                    *childID       = children[i];
                }
                break;
            }
            case R_Transform_STALE_WORLD:
            case R_Transform_STALE_LOCAL: {
                // get parent world matrix
                R_Transform* parent = Component_GetXform(xform->parentID);
                _Transform_RebuildDescendants(xform, parent ? &parent->world :
                                                              &identityMat);
                break;
            }
            default: log_error("unhandled staleness %d", xform->_stale); break;
        }

        // always set fresh
        xform->_stale = R_Transform_STALE_NONE;

        // update stack top
        top = (SG_ID*)Arena::top(arena);
    }
}

u32 R_Transform::numChildren(R_Transform* xform)
{
    return ARENA_LENGTH(&xform->children, SG_ID);
}

R_Transform* R_Transform::getChild(R_Transform* xform, u32 index)
{
    return Component_GetXform(*ARENA_GET_TYPE(&xform->children, SG_ID, index));
}

void R_Transform::rotateOnLocalAxis(R_Transform* xform, glm::vec3 axis, f32 deg)
{
    R_Transform::rot(xform,
                     xform->_rot * glm::angleAxis(deg, glm::normalize(axis)));
}

void R_Transform::rotateOnWorldAxis(R_Transform* xform, glm::vec3 axis, f32 deg)
{
    R_Transform::rot(xform,
                     glm::angleAxis(deg, glm::normalize(axis)) * xform->_rot);
}

void R_Transform::print(R_Transform* xform, u32 depth)
{
    for (u32 i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("%s\n", xform->name.c_str());
    // print position
    for (u32 i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("  pos: %f %f %f\n", xform->_pos.x, xform->_pos.y, xform->_pos.z);

    u32 childrenCount = R_Transform::numChildren(xform);
    for (u32 i = 0; i < childrenCount; ++i) {
        print(R_Transform::getChild(xform, i), depth + 1);
    }
}

// ============================================================================
// Geometry Component
// ============================================================================
void R_Geometry::init(R_Geometry* geo)
{
    ASSERT(geo->id == 0);
    *geo = {};

    geo->id   = getNewComponentID();
    geo->type = SG_COMPONENT_GEOMETRY;
}

void R_Geometry::buildFromVertices(GraphicsContext* gctx, R_Geometry* geo,
                                   Vertices* vertices)
{
    ASSERT(geo->gpuIndexBuffer == NULL);
    ASSERT(geo->gpuVertexBuffer == NULL);

    // TODO: optimization. Flag if tangents are already present
    Vertices::buildTangents(vertices);

    // write indices
    if (vertices->indicesCount > 0) {
        geo->indexBufferDesc.usage
          = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
        geo->indexBufferDesc.size = sizeof(u32) * vertices->indicesCount;
        geo->gpuIndexBuffer
          = wgpuDeviceCreateBuffer(gctx->device, &geo->indexBufferDesc);

        wgpuQueueWriteBuffer(gctx->queue, geo->gpuIndexBuffer, 0,
                             vertices->indices, geo->indexBufferDesc.size);
    }

    { // write vertices
        geo->vertexBufferDesc.usage
          = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
        geo->vertexBufferDesc.size
          = CHUGL_FLOATS_PER_VERTEX * sizeof(f32) * vertices->vertexCount;
        geo->gpuVertexBuffer
          = wgpuDeviceCreateBuffer(gctx->device, &geo->vertexBufferDesc);

        wgpuQueueWriteBuffer(gctx->queue, geo->gpuVertexBuffer, 0,
                             vertices->vertexData, geo->vertexBufferDesc.size);
    }

    geo->numVertices = vertices->vertexCount;
    geo->numIndices  = vertices->indicesCount;

    log_trace("geo->numVertices: %d", geo->numVertices);
    log_trace("geo->numIndices: %d", geo->numIndices);
}

// ============================================================================
// R_Texture
// ============================================================================

void R_Texture::init(R_Texture* texture)
{
    ASSERT(texture->id == 0);
    *texture      = {};
    texture->id   = getNewComponentID();
    texture->type = SG_COMPONENT_TEXTURE;
}

// ============================================================================
// Material_Primitive
// ============================================================================

void Material_Primitive::init(Material_Primitive* prim, R_Material* mat,
                              SG_ID geoID)
{
    ASSERT(prim->geoID == 0);
    ASSERT(prim->matID == 0);
    ASSERT(prim->xformIDs.cap == 0);

    prim->geoID = geoID;
    prim->matID = mat->id;

    Arena::init(&prim->xformIDs, 8 * sizeof(SG_ID));
}

void Material_Primitive::free(Material_Primitive* prim)
{
    // free arena
    Arena::free(&prim->xformIDs);

    // free webgpu resources
    // TODO: consider using macro for all create/release
    // track counters in debug mode
    // (like leakcheck but for webgpu resources)
    wgpuBindGroupRelease(prim->bindGroup);
    wgpuBufferDestroy(prim->storageBuffer);
    wgpuBufferRelease(prim->storageBuffer);

    // zero fields
    *prim = {};
}

u32 Material_Primitive::numInstances(Material_Primitive* prim)
{
    return ARENA_LENGTH(&prim->xformIDs, SG_ID);
}

void Material_Primitive::addXform(Material_Primitive* prim, R_Transform* xform)
{
// check we aren't inserting a duplicate
#ifdef CHUGL_DEBUG
    for (u32 i = 0; i < ARENA_LENGTH(&prim->xformIDs, SG_ID); ++i) {
        if (*ARENA_GET_TYPE(&prim->xformIDs, SG_ID, i) == xform->id) {
            log_error("xform already exists in geometry");
            return;
        }
    }
#endif
    // if xform is already set to this primitive, do nothing
    if (xform->_geoID == prim->geoID && xform->_matID == prim->matID) return;

    // set stale
    prim->stale = true;

    // remove xform from previous material
    if (xform->_geoID && xform->_matID) {
        R_Material* prevMat = Component_GetMaterial(xform->_matID);
        R_Geometry* prevGeo = Component_GetGeo(xform->_geoID);
        ASSERT(prevMat && prevGeo); // reference counting should prevent geo
                                    // from being deleted
        R_Material::removePrimitive(prevMat, prevGeo, xform);
    }

    // add xform to new primitive
    xform->_geoID  = prim->geoID;
    xform->_matID  = prim->matID;
    SG_ID* xformID = ARENA_PUSH_TYPE(&prim->xformIDs, SG_ID);
    *xformID       = xform->id;

    // TODO refcounting
}

void Material_Primitive::removeXform(Material_Primitive* prim,
                                     R_Transform* xform)
{
    u64 numInstances = Material_Primitive::numInstances(prim);
    SG_ID* instances = (SG_ID*)prim->xformIDs.base;
    for (u32 i = 0; i < numInstances; ++i) {
        SG_ID id = instances[i];
        if (id == xform->id) {
            // swap with last element
            instances[i] = instances[numInstances - 1];
            // pop last element
            Arena::popZero(&prim->xformIDs, sizeof(SG_ID));
            // set stale
            prim->stale = true;
            break;
        }
    }
}

void Material_Primitive::rebuildBindGroup(GraphicsContext* gctx,
                                          Material_Primitive* prim,
                                          WGPUBindGroupLayout layout,
                                          Arena* arena)
{
    if (!prim->stale) return;

    // TODO: in transform delete, remember to detach it from its material
    // primitive

    // log_trace("rebuilding bindgroup for primitive with geo %d", geo->id);
    prim->stale = false;

    // build new array of matrices on CPU
    glm::mat4* modelMatrices = (glm::mat4*)Arena::top(arena);
    u32 matrixCount          = 0;

    u64 numInstances = Material_Primitive::numInstances(prim);
    SG_ID* xformIDs  = (SG_ID*)prim->xformIDs.base;
    // delete and swap any destroyed xforms
    for (u32 i = 0; i < numInstances; ++i) {
        R_Transform* xform = Component_GetXform(xformIDs[i]);
        // remove NULL xforms via swapping
        if (xform == NULL) {
            // swap with last element
            xformIDs[i] = xformIDs[numInstances - 1];
            // pop last element
            Arena::pop(&prim->xformIDs, sizeof(SG_ID));
            // decrement to reprocess this index
            --i;
            --numInstances;
            continue;
        }
        // assert his xform belongs to this material and geometry
        ASSERT(xform->_geoID == prim->geoID);
        ASSERT(xform->_matID == prim->matID);
        // else add xform matrix to arena
        ++matrixCount;
        // world matrix should already have been computed by now
        ASSERT(xform->_stale == R_Transform_STALE_NONE);
        glm::mat4* matPtr = ARENA_PUSH_TYPE(arena, glm::mat4);
        *matPtr           = xform->world;
    }
    // sanity check that we have the correct number of matrices
    ASSERT(matrixCount == Material_Primitive::numInstances(prim));
    // update numInstances after possibly deleting some
    numInstances = matrixCount;

    // rebuild storage buffer only if it needs to grow
    if (numInstances > prim->storageBufferCap) {
        // double capacity (like dynamic array)
        u32 newCap = MAX(numInstances, prim->storageBufferCap * 2);
        // round newCap to largest power of 2
        newCap = NEXT_POW2(newCap);

        log_trace(
          "growing storage buffer for geometry %d from capacity: %d to "
          "%d",
          prim->geoID, prim->storageBufferCap, newCap);

        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size                 = sizeof(DrawUniforms) * newCap;
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage;
        // destroy previous
        if (prim->storageBuffer != NULL) {
            wgpuBufferDestroy(prim->storageBuffer);
            wgpuBufferRelease(prim->storageBuffer);
        }
        prim->storageBuffer = wgpuDeviceCreateBuffer(gctx->device, &bufferDesc);
        // update new capacity
        prim->storageBufferCap = newCap;

        // create new bindgroup for new storage buffer
        {
            prim->bindGroupEntry         = {};
            prim->bindGroupEntry.binding = 0; // @binding(0)
            prim->bindGroupEntry.offset  = 0;
            prim->bindGroupEntry.buffer
              = prim->storageBuffer; // only supports uniform buffers for now
            prim->bindGroupEntry.size = bufferDesc.size;

            // release previous
            if (prim->bindGroup) wgpuBindGroupRelease(prim->bindGroup);

            // A bind group contains one or multiple bindings
            WGPUBindGroupDescriptor desc = {};
            desc.layout                  = layout;
            desc.entries                 = &prim->bindGroupEntry;
            desc.entryCount              = 1; // force 1 binding per group
            prim->bindGroup = wgpuDeviceCreateBindGroup(gctx->device, &desc);
        }
    }

    // populate storage buffer with new xform data
    wgpuQueueWriteBuffer(gctx->queue, prim->storageBuffer, 0, modelMatrices,
                         matrixCount * sizeof(modelMatrices[0]));

    // pop arena after copying data to GPU
    ARENA_POP_COUNT(arena, glm::mat4, matrixCount);
    ASSERT(modelMatrices
           == Arena::top(arena)); // make sure arena is back to original state
}

// ============================================================================
// R_Material
// ============================================================================

void MaterialTextureView::init(MaterialTextureView* view)
{
    *view          = {};
    view->strength = 1.0f;
    view->scale[0] = 1.0f;
    view->scale[1] = 1.0f;
}

static void _R_Material_Init(R_Material* mat)
{
    ASSERT(mat->id == 0);
    ASSERT(mat->pipelineID == 0);
    *mat       = {};
    mat->id    = getNewComponentID();
    mat->type  = SG_COMPONENT_MATERIAL;
    mat->stale = true;

    // initialize primitives arena
    Arena::init(&mat->primitives, sizeof(Material_Primitive) * 8);

    // initialize bind entry arenas
    Arena::init(&mat->bindings, sizeof(R_Binding) * 8);
    Arena::init(&mat->bindGroupEntries, sizeof(WGPUBindGroupEntry) * 8);
    Arena::init(&mat->uniformData, 64);
}

void R_Material::init(GraphicsContext* gctx, R_Material* mat,
                      R_MaterialConfig* config)
{
    _R_Material_Init(mat);

    // copy config (TODO do we even need to save this? it's stored in the
    // pipeline anyways)
    mat->config = *config;

    // get pipeline
    R_RenderPipeline* pipeline = Component_GetPipeline(gctx, config);

    // add material to pipeline
    R_RenderPipeline::addMaterial(pipeline, mat);
    ASSERT(mat->pipelineID != 0);

    // init texture views
    // MaterialTextureView::init(&mat->baseColorTextureView);
    // MaterialTextureView::init(&mat->metallicRoughnessTextureView);
    // MaterialTextureView::init(&mat->normalTextureView);

    // init base color to white
    // mat->baseColor = glm::vec4(1.0f);
}

static void _R_Material_BindGroupEntryFromBinding(GraphicsContext* gctx,
                                                  R_Material* mat,
                                                  R_Binding* binding,
                                                  WGPUBindGroupEntry* entry,
                                                  u32 bindIndex)
{
    *entry         = {};
    entry->binding = bindIndex;

    switch (binding->type) {
        case R_BIND_UNIFORM: {
            entry->offset = binding->as.uniformOffset;
            entry->size   = binding->size;
            entry->buffer = mat->gpuUniformBuff;
            break;
        }
        case R_BIND_SAMPLER: {
            entry->sampler
              = Graphics_GetSampler(gctx, binding->as.samplerConfig);
            break;
        }
        case R_BIND_TEXTURE_ID: {
            R_Texture* rTexture = Component_GetTexture(binding->as.textureID);
            // TODO: default textures
            if (rTexture == NULL) {
                ASSERT(false);
                log_error("texture not found for binding");
                return;
            }
            entry->textureView = rTexture->gpuTexture.view;
            break;
        }
        case R_BIND_TEXTURE_VIEW: {
            entry->textureView = binding->as.textureView;
            break;
        };
        case R_BIND_STORAGE:
        default: {
            ASSERT(false);
            log_error("unsupported binding type %d", binding->type);
            break;
        }
    }
}

void R_Material::rebuildBindGroup(R_Material* mat, GraphicsContext* gctx,
                                  WGPUBindGroupLayout layout)
{
    if (!mat->stale) return;

    mat->stale = false;

    // rebuild uniform buffer
    // TODO: only need to do this if a UNIFORM binding is stale
    {
        size_t uniformDataSize = ARENA_LENGTH(&mat->uniformData, u8);

        // maybe put this logic in uniformBuffer class in graphics.h
        if (uniformDataSize > mat->uniformBuffDesc.size) {
            // free previous buffer
            WGPU_DESTROY_AND_RELEASE_BUFFER(mat->gpuUniformBuff);
            // create new descriptor
            mat->uniformBuffDesc = {};
            mat->uniformBuffDesc.usage
              = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
            mat->uniformBuffDesc.size = uniformDataSize;
            // create buffer
            mat->gpuUniformBuff
              = wgpuDeviceCreateBuffer(gctx->device, &mat->uniformBuffDesc);
        }

        // write data cpu -> gpu
        wgpuQueueWriteBuffer(gctx->queue, mat->gpuUniformBuff, 0,
                             mat->uniformData.base, uniformDataSize);
    }

    // TODO: rebuild storageBuffer for storage bindings
    // will need to store a WGPUBuffer for each storage binding
    // for now unsupported

    // create bindgroups for all bindings
    u32 numBindings = ARENA_LENGTH(&mat->bindings, R_Binding);
    u32 numBindGroupEntries
      = ARENA_LENGTH(&mat->bindGroupEntries, WGPUBindGroupEntry);
    ASSERT(numBindings == numBindGroupEntries); // WGPUBindGroupEntry should
                                                // exist for each R_Binding

    for (u32 i = 0; i < numBindings; ++i) {
        R_Binding* binding = ARENA_GET_TYPE(&mat->bindings, R_Binding, i);
        WGPUBindGroupEntry* entryDesc
          = ARENA_GET_TYPE(&mat->bindGroupEntries, WGPUBindGroupEntry, i);
        _R_Material_BindGroupEntryFromBinding(gctx, mat, binding, entryDesc, i);
    }

    // release previous bindgroup
    WGPU_RELEASE_RESOURCE(BindGroup, mat->bindGroup);

    // get old pipeline
    R_RenderPipeline* pipeline = Component_GetPipeline(mat->pipelineID);
    ASSERT(pipeline);

    // create new bindgroup
    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout                  = layout;
    bgDesc.entryCount              = numBindGroupEntries;
    bgDesc.entries = (WGPUBindGroupEntry*)mat->bindGroupEntries.base;
    mat->bindGroup = wgpuDeviceCreateBindGroup(gctx->device, &bgDesc);
}

// TODO: this function can take an R_Binding directly, rather than
// type/data/bytes.
// Or maybe break this into setUniform, setTexture, setSampler, etc.
// to simplify the switch statement logic
void R_Material::setBinding(R_Material* mat, u32 location, R_BindType type,
                            void* data, size_t bytes)
{
    mat->stale = true;

    // allocate memory in arenas
    {
        u32 numBindings = ARENA_LENGTH(&mat->bindings, R_Binding);
        u32 numbindGroups
          = ARENA_LENGTH(&mat->bindGroupEntries, WGPUBindGroupEntry);
        ASSERT(numBindings == numbindGroups);
        if (numBindings < location + 1) {
            // grow bindings arena
            ARENA_PUSH_COUNT(&mat->bindings, R_Binding,
                             location + 1 - numBindings);
            // grow bindgroup entries arena
            ARENA_PUSH_COUNT(&mat->bindGroupEntries, WGPUBindGroupEntry,
                             location + 1 - numbindGroups);
        }
    }

    { // replace previous binding
        // NOTE: currently don't allow changing binding type for a given
        // location
        // NOTE: don't allow changing binding size for UNIFORM bindings
        // at same location
        R_Binding* binding
          = ARENA_GET_TYPE(&mat->bindings, R_Binding, location);
        ASSERT(binding->type == R_BIND_EMPTY || binding->type == type);
        R_BindType prevType = binding->type;
        size_t prevSize     = binding->size;
        // create new binding
        switch (type) {
            case R_BIND_UNIFORM: {
                // get offset in uniform buffer
                ASSERT(prevSize == 0 || prevSize == bytes);

                // first time setting this bindgroup, allocate memory for
                // uniform
                if (prevType == R_BIND_EMPTY) {
                    u8* uniformPtr
                      = ARENA_PUSH_COUNT(&mat->uniformData, u8, bytes);
                    // set offset pointer
                    binding->as.uniformOffset
                      = Arena::offsetOf(&mat->uniformData, uniformPtr);
                }

                u8* uniformPtr = ARENA_GET_TYPE(&mat->uniformData, u8,
                                                binding->as.uniformOffset);

                // write new data
                memcpy(uniformPtr, data, bytes);
                break;
            }
            case R_BIND_TEXTURE_ID: {
                ASSERT(bytes == sizeof(SG_ID));
                binding->as.textureID = *(SG_ID*)data;
                // TODO refcount
                break;
            }
            case R_BIND_TEXTURE_VIEW: {
                ASSERT(bytes == sizeof(WGPUTextureView));
                binding->as.textureView = *(WGPUTextureView*)data;
                break;
            }
            case R_BIND_SAMPLER: {
                ASSERT(bytes == sizeof(SamplerConfig));
                binding->as.samplerConfig = *(SamplerConfig*)data;
                break;
            }
            case R_BIND_STORAGE:
            default:
                // if the new binding is also STORAGE reuse the memory, don't
                // free
                log_error("unsupported binding type %d", type);
                ASSERT(false);
                break;
        }

        binding->type = type;
        binding->size = bytes;
    }
}

// assumes sampler is at location+1
void R_Material::setTextureAndSamplerBinding(R_Material* mat, u32 location,
                                             SG_ID textureID,
                                             WGPUTextureView defaultView)
{
    SamplerConfig defaultSampler = {};

    R_Texture* rTexture = Component_GetTexture(textureID);

    if (rTexture) {
        R_Material::setBinding(mat, location, R_BIND_TEXTURE_ID, &textureID,
                               sizeof(SG_ID));
        // set diffuse texture sampler
        R_Material::setBinding(mat, location + 1, R_BIND_SAMPLER,
                               &rTexture->samplerConfig, sizeof(SamplerConfig));
    } else {
        // use default white pixel texture
        R_Material::setBinding(mat, location, R_BIND_TEXTURE_VIEW, &defaultView,
                               sizeof(WGPUTextureView));
        // set default sampler
        R_Material::setBinding(mat, location + 1, R_BIND_SAMPLER,
                               &defaultSampler, sizeof(SamplerConfig));
    }
}

u32 R_Material::numPrimitives(R_Material* mat)
{
    // TODO: after optimizing with hashmap plus
    // active / empty regions, may need to change this to a variable
    return ARENA_LENGTH(&mat->primitives, Material_Primitive);
}

// TODO: using array linear search for now
// to improve: use array + hashmap combo for fast lookup
// and linear iteration
void R_Material::addPrimitive(R_Material* mat, R_Geometry* geo,
                              R_Transform* xform)
{
    // check if primitive for geoID already exists
    u32 numPrims = R_Material::numPrimitives(mat);
    for (u32 i = 0; i < numPrims; ++i) {
        Material_Primitive* prim
          = ARENA_GET_TYPE(&mat->primitives, Material_Primitive, i);
        if (prim->geoID == geo->id) {
            Material_Primitive::addXform(prim, xform);
            ASSERT(prim->stale);
            return;
        }
    }
    // else create new primitive for this geometry
    Material_Primitive* prim
      = ARENA_PUSH_TYPE(&mat->primitives, Material_Primitive);
    Material_Primitive::init(prim, mat, geo->id);

    // add xform to new primitive
    Material_Primitive::addXform(prim, xform);

    // check ids were set correctly
    ASSERT(xform->_geoID == geo->id);
    ASSERT(xform->_matID == mat->id);
    ASSERT(prim->stale);

    // TODO: reference counting (or maybe only need in Material::Primitive
    // addXform / removeXform)
}

/// @brief remove instance of xform from the geo primitive on mat
// TODO: accelerate with hashmap
void R_Material::removePrimitive(R_Material* mat, R_Geometry* geo,
                                 R_Transform* xform)
{
    ASSERT(xform->_geoID == geo->id && xform->_matID == mat->id);

    u32 numPrims = R_Material::numPrimitives(mat);
    for (u32 i = 0; i < numPrims; ++i) {
        Material_Primitive* prim
          = ARENA_GET_TYPE(&mat->primitives, Material_Primitive, i);
        if (prim->geoID == geo->id) {
            Material_Primitive::removeXform(prim, xform);
            ASSERT(prim->stale);
            // empty primitive, either remove or swap with last
            if (Material_Primitive::numInstances(prim) == 0) {
                // TODO: figure out how to handle this.
                // for now, leaving in place
            }
            return;
        }
    }
}

// Linear search
// TODO use hashmap later
static Material_Primitive* _R_Material_GetPrimitive(R_Material* mat,
                                                    SG_ID geoID)
{
    u32 numPrims = R_Material::numPrimitives(mat);
    for (u32 i = 0; i < numPrims; ++i) {
        Material_Primitive* prim
          = ARENA_GET_TYPE(&mat->primitives, Material_Primitive, i);
        if (prim->geoID == geoID) return prim;
    }
    return NULL;
}

void R_Material::markPrimitiveStale(R_Material* mat, R_Transform* xform)
{
    ASSERT(xform->_matID == mat->id);
    if (xform->_geoID == 0) return;

    // find the primitive that this xform belongs to
    Material_Primitive* prim = _R_Material_GetPrimitive(mat, xform->_geoID);
    ASSERT(prim != NULL);
    prim->stale = true;
}

bool R_Material::primitiveIter(R_Material* mat, size_t* indexPtr,
                               Material_Primitive** primitive)
{
    u32 numPrimitives = ARENA_LENGTH(&mat->primitives, Material_Primitive);
    if (*indexPtr >= numPrimitives) {
        *primitive = NULL;
        return false;
    }

    // possible optimization:
    // lazily delete / swap empty primitives in this iter loop.
    // for now just doing a linear walk

    *primitive
      = ARENA_GET_TYPE(&mat->primitives, Material_Primitive, *indexPtr);

    ASSERT(*primitive != NULL);
    ASSERT((*primitive)->matID == mat->id);

    ++(*indexPtr);
    return true;
}

// ============================================================================
// R_Scene
// ============================================================================

void R_Scene::initFromSG(R_Scene* r_scene, SG_Command_SceneCreate* cmd)
{
    ASSERT(r_scene->id == 0); // ensure not initialized twice
    *r_scene = {};

    // copy base component data
    // TODO have a separate R_ComponentType enum?
    r_scene->id   = cmd->sg_id;
    r_scene->type = SG_COMPONENT_SCENE;

    // copy xform
    r_scene->_pos = VEC_ORIGIN;
    r_scene->_rot = QUAT_IDENTITY;
    r_scene->_sca = VEC_ONES;

    // set stale to force rebuild of matrices
    r_scene->_stale = R_Transform_STALE_LOCAL;

    // initialize children array for 8 children
    Arena::init(&r_scene->children, sizeof(SG_ID) * 8);
}

// ============================================================================
// Render Pipeline Definitions
// ============================================================================

void R_RenderPipeline::init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
                            const R_MaterialConfig* config, ptrdiff_t offset)
{
    ASSERT(pipeline->pipeline.pipeline == NULL);
    ASSERT(pipeline->rid == 0);

    pipeline->rid = getNewRID();
    ASSERT(pipeline->rid < 0);

    memcpy(&pipeline->config, config, sizeof(R_MaterialConfig));
    pipeline->offset = offset;

    const char* vertShaderCode = NULL;
    const char* fragShaderCode = NULL;
    switch (config->material_type) {
        case SG_MATERIAL_PBR: {
            vertShaderCode = shaderCode;
            fragShaderCode = shaderCode;
            break;
        }
        default: ASSERT(false && "unsupported shader type");
    }

    RenderPipeline::init(gctx, &pipeline->pipeline, vertShaderCode,
                         fragShaderCode);

    Arena::init(&pipeline->materialIDs, sizeof(SG_ID) * 8);
}

/*
Tricky: a material can only ever be bound to a single pipeline,
BUT it might switch pipelines over the course of its lifetime.
E.g. the user switches a material from being single-sided to double-sided,
or from being opaque to transparent.
How to support this?

Add a function R_MaterialConfig GetRenderPipelineConfig() to R_Material

Option 1: before rendering, do a pass over all materials in material arena.
Get its renderPipelineConfig, and see if its tied to the correct pipeline.
If not, remove from the current and add to the new.
- cons: requires a second pass over all materials, which is not cache efficient

Option 2: check the material WHEN doing the render pass over all RenderPipeline.
Same logic, GetRenderPipelineConfig() and if its different, add to correct
pipeline.
- cons: if the material gets added to a pipeline that was ALREADY processed, it
  will be missed and only rendered on the NEXT frame. (that might be fine)

Going with option 2 for now

*/

void R_RenderPipeline::addMaterial(R_RenderPipeline* pipeline,
                                   R_Material* material)
{
    ASSERT(material);
    // disallow duplicates
    ASSERT(material->pipelineID != pipeline->rid);

    // remove material from existing pipeline
    // Lazy deletion. during render loop, pipeline checks if the material
    // ID matches this current pipeline. If not, we know we've switched.
    // the render pipeline then removes the material from its list of material
    // IDs.
    // This avoids the cost of doing a linear search over all materials for
    // deletion. We postpone the deletion until we're already iterating over the
    // materials anyways

    // TODO: remember to remove NULL material IDs from the list, AS WELL AS
    // materials whose pipeline id doesn't match

    // add material to new pipeline
    SG_ID* newMaterialID = ARENA_PUSH_TYPE(&pipeline->materialIDs, SG_ID);
    *newMaterialID       = material->id;
    material->pipelineID = pipeline->rid;
}

// While iterating, will lazy delete NULL material IDs and materials
// whose config doesn't match the pipeline (e.g. material has been assigned
// elsewhere) swap NULL with last element and pop
// Can do away with this lazy deletion if we augment the SG_ID arena with a
// hashmap for O(1) material ID lookup and deletion
bool R_RenderPipeline::materialIter(R_RenderPipeline* pipeline,
                                    size_t* indexPtr, R_Material** material)
{
    size_t numMaterials = ARENA_LENGTH(&pipeline->materialIDs, SG_ID);

    if (*indexPtr >= numMaterials) {
        *material = NULL;
        return false;
    }

    SG_ID* materialID
      = ARENA_GET_TYPE(&pipeline->materialIDs, SG_ID, *indexPtr);
    *material = Component_GetMaterial(*materialID);

    // if null or reassigned to different pipeline, swap with last element and
    // try again
    if (*material == NULL || (*material)->pipelineID != pipeline->rid) {
        SG_ID* lastMatID
          = ARENA_GET_TYPE(&pipeline->materialIDs, SG_ID, numMaterials - 1);

        // swap
        *materialID = *lastMatID;
        // pop last element
        Arena::pop(&pipeline->materialIDs, sizeof(SG_ID));
        // try again with same index
        return materialIter(pipeline, indexPtr, material);
    }

    // else return normally
    ++(*indexPtr);
    return true;
}

// ============================================================================
// Component Manager Definitions
// ============================================================================

// storage arenas
static Arena xformArena;
static Arena sceneArena;
static Arena geoArena;
static Arena materialArena;
static Arena textureArena;
static Arena _RenderPipelineArena;

// default textures
static Texture opaqueWhitePixel      = {};
static Texture transparentBlackPixel = {};
static Texture defaultNormalPixel    = {};

// maps from id --> offset
static hashmap* r_locator = NULL;
std::unordered_map<R_ID, u64> _RenderPipelineMap;

struct R_Location {
    SG_ID id;      // key
    size_t offset; // value (byte offset into arena)
    Arena* arena;  // where to find
};

static int R_CompareLocation(const void* a, const void* b, void* udata)
{
    R_Location* locA = (R_Location*)a;
    R_Location* locB = (R_Location*)b;
    return locA->id - locB->id;
}

static u64 R_HashLocation(const void* item, uint64_t seed0, uint64_t seed1)
{
    // tested xxhash3 is best
    R_Location* key = (R_Location*)item;
    return hashmap_xxhash3(&key->id, sizeof(SG_ID), seed0, seed1);
    // return hashmap_sip(item, sizeof(int), seed0, seed1);
    // return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

void Component_Init(GraphicsContext* gctx)
{
    // initialize arena memory
    Arena::init(&xformArena, sizeof(R_Transform) * 128);
    Arena::init(&sceneArena, sizeof(R_Scene) * 128);
    Arena::init(&geoArena, sizeof(R_Geometry) * 128);
    Arena::init(&materialArena, sizeof(R_Material) * 64);
    Arena::init(&_RenderPipelineArena, sizeof(R_RenderPipeline) * 8);
    Arena::init(&textureArena, sizeof(R_Texture) * 64);

    // initialize default textures
    static u8 white[4]  = { 255, 255, 255, 255 };
    static u8 black[4]  = { 0, 0, 0, 0 };
    static u8 normal[4] = { 128, 128, 255, 255 };
    Texture::initSinglePixel(gctx, &opaqueWhitePixel, white);
    Texture::initSinglePixel(gctx, &transparentBlackPixel, black);
    Texture::initSinglePixel(gctx, &defaultNormalPixel, normal);

    // init locator
    int seed = time(NULL);
    srand(seed);
    r_locator = hashmap_new(sizeof(R_Location), 0, seed, seed, R_HashLocation,
                            R_CompareLocation, NULL, NULL);
}

void Component_Free()
{
    // TODO: should we also free the individual components?

    // free arena memory
    Arena::free(&xformArena);
    Arena::free(&sceneArena);
    Arena::free(&geoArena);
    Arena::free(&materialArena);
    Arena::free(&_RenderPipelineArena);
    Arena::free(&textureArena);

    // free default textures
    Texture::release(&opaqueWhitePixel);
    Texture::release(&transparentBlackPixel);
    Texture::release(&defaultNormalPixel);

    // free locator
    hashmap_free(r_locator);
    r_locator = NULL;
}

R_Transform* Component_CreateTransform()
{
    R_Transform* xform = ARENA_PUSH_TYPE(&xformArena, R_Transform);
    R_Transform::init(xform);

    ASSERT(xform->id != 0);                        // ensure id is set
    ASSERT(xform->type == SG_COMPONENT_TRANSFORM); // ensure type is set

    // store offset
    R_Location loc
      = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd)
{
    R_Transform* xform = ARENA_PUSH_TYPE(&xformArena, R_Transform);
    R_Transform::initFromSG(xform, cmd);

    ASSERT(xform->id != 0);                        // ensure id is set
    ASSERT(xform->type == SG_COMPONENT_TRANSFORM); // ensure type is set

    // store offset
    R_Location loc
      = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Scene* Component_CreateScene(SG_Command_SceneCreate* cmd)
{
    R_Scene* r_scene = ARENA_PUSH_TYPE(&xformArena, R_Scene);
    R_Scene::initFromSG(r_scene, cmd);

    ASSERT(r_scene->id != 0);                    // ensure id is set
    ASSERT(r_scene->type == SG_COMPONENT_SCENE); // ensure type is set

    // store offset
    R_Location loc
      = { r_scene->id, Arena::offsetOf(&sceneArena, r_scene), &sceneArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return r_scene;
}

R_Geometry* Component_CreateGeometry()
{
    R_Geometry* geo = ARENA_PUSH_TYPE(&geoArena, R_Geometry);
    R_Geometry::init(geo);

    ASSERT(geo->id != 0);                       // ensure id is set
    ASSERT(geo->type == SG_COMPONENT_GEOMETRY); // ensure type is set

    // store offset
    R_Location loc = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return geo;
}

R_Geometry* Component_CreateGeometry(GraphicsContext* gctx,
                                     SG_Command_GeoCreate* cmd)
{
    // first build vertices
    Vertices vertices;
    switch (cmd->geo_type) {
        case SG_GEOMETRY_PLANE: {
            Vertices::createPlane(&vertices, &cmd->params.plane);
            break;
        }
        default: ASSERT(false)
    }

    R_Geometry* geo = ARENA_PUSH_TYPE(&geoArena, R_Geometry);
    R_Geometry::buildFromVertices(gctx, geo, &vertices);

    geo->id   = cmd->sg_id;
    geo->type = SG_COMPONENT_GEOMETRY;
    // for now not storing geo_type (cube, sphere, custom etc.)
    // we only store the GPU vertex data, and don't care about semantics

    // store offset
    R_Location loc = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return geo;
}

R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     R_MaterialConfig* config)
{
    R_Material* mat = ARENA_PUSH_TYPE(&materialArena, R_Material);
    R_Material::init(gctx, mat, config);

    ASSERT(mat->id != 0);                       // ensure id is set
    ASSERT(mat->type == SG_COMPONENT_MATERIAL); // ensure type is set

    // store offset
    R_Location loc
      = { mat->id, Arena::offsetOf(&materialArena, mat), &materialArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return mat;
}

R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd)
{
    R_Material* mat = ARENA_PUSH_TYPE(&materialArena, R_Material);

    // initialize
    {
        *mat       = {};
        mat->id    = cmd->sg_id;
        mat->type  = SG_COMPONENT_MATERIAL;
        mat->stale = true;

        // build config
        // TODO: pass config from SG_Material
        mat->config               = {};
        mat->config.material_type = cmd->material_type;

        // set material params/uniforms
        // TODO this only works for PBR for now
        {
            // TODO maybe consolidate SG_MaterialParams and R_MaterialParams
            // make the SG_MaterialParams struct alignment work for webgpu
            // uniforms
            MaterialUniforms pbr_uniforms  = {};
            SG_Material_PBR_Params* params = &cmd->params.pbr;
            pbr_uniforms.baseColor         = params->baseColor;
            pbr_uniforms.emissiveFactor    = params->emissiveFactor;
            pbr_uniforms.metallic          = params->metallic;
            pbr_uniforms.roughness         = params->roughness;
            pbr_uniforms.normalFactor      = params->normalFactor;
            pbr_uniforms.aoFactor          = params->aoFactor;

            R_Material::setBinding(mat, 0, R_BIND_UNIFORM, &pbr_uniforms,
                                   sizeof(pbr_uniforms));

            R_Material::setTextureAndSamplerBinding(mat, 1, 0,
                                                    opaqueWhitePixel.view);

            R_Material::setTextureAndSamplerBinding(mat, 3, 0,
                                                    defaultNormalPixel.view);

            R_Material::setTextureAndSamplerBinding(mat, 5, 0,
                                                    opaqueWhitePixel.view);

            R_Material::setTextureAndSamplerBinding(mat, 7, 0,
                                                    opaqueWhitePixel.view);
        }

        // initialize primitives arena
        Arena::init(&mat->primitives, sizeof(Material_Primitive) * 8);

        // initialize bind entry arenas
        Arena::init(&mat->bindings, sizeof(R_Binding) * 8);
        Arena::init(&mat->bindGroupEntries, sizeof(WGPUBindGroupEntry) * 8);
        Arena::init(&mat->uniformData, 64);

        // get pipeline
        R_RenderPipeline* pipeline = Component_GetPipeline(gctx, &mat->config);

        // add material to pipeline
        R_RenderPipeline::addMaterial(pipeline, mat);
        ASSERT(mat->pipelineID != 0);
    }

    // store offset
    R_Location loc
      = { mat->id, Arena::offsetOf(&materialArena, mat), &materialArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return mat;
}

R_Texture* Component_CreateTexture()
{
    R_Texture* texture = ARENA_PUSH_TYPE(&textureArena, R_Texture);
    R_Texture::init(texture);

    ASSERT(texture->id != 0);                      // ensure id is set
    ASSERT(texture->type == SG_COMPONENT_TEXTURE); // ensure type is set

    // store offset
    R_Location loc
      = { texture->id, Arena::offsetOf(&textureArena, texture), &textureArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return texture;
}

R_Component* Component_GetComponent(SG_ID id)
{
    R_Location loc     = { id, 0, NULL };
    R_Location* result = (R_Location*)hashmap_get(r_locator, &loc);
    return result ? (R_Component*)Arena::get(result->arena, result->offset) :
                    NULL;
}

R_Transform* Component_GetXform(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    if (comp) {
        ASSERT(comp->type == SG_COMPONENT_TRANSFORM
               || comp->type == SG_COMPONENT_SCENE);
    }
    return (R_Transform*)comp;
}

R_Scene* Component_GetScene(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_SCENE);
    return (R_Scene*)comp;
}

R_Geometry* Component_GetGeo(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_GEOMETRY);
    return (R_Geometry*)comp;
}

R_Material* Component_GetMaterial(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_MATERIAL);
    return (R_Material*)comp;
}

R_Texture* Component_GetTexture(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_TEXTURE);
    return (R_Texture*)comp;
}

bool Component_MaterialIter(size_t* i, R_Material** material)
{
    if (*i >= ARENA_LENGTH(&materialArena, R_Material)) {
        *material = NULL;
        return false;
    }

    *material = ARENA_GET_TYPE(&materialArena, R_Material, *i);
    ++(*i);
    return true;
}

bool Component_RenderPipelineIter(size_t* i, R_RenderPipeline** renderPipeline)
{
    if (*i >= ARENA_LENGTH(&_RenderPipelineArena, R_RenderPipeline)) {
        *renderPipeline = NULL;
        return false;
    }

    // Possible optimization: pack nonempty pipelines at start, swap empty
    // pipelines to end
    *renderPipeline
      = ARENA_GET_TYPE(&_RenderPipelineArena, R_RenderPipeline, *i);
    ++(*i);
    return true;
}

R_RenderPipeline* Component_GetPipeline(GraphicsContext* gctx,
                                        R_MaterialConfig* config)
{
    // TODO figure out a better way to search (augment with a hashmap?)
    // for now just doing linear search

    u32 numPipelines = ARENA_LENGTH(&_RenderPipelineArena, R_RenderPipeline);
    for (u32 i = 0; i < numPipelines; ++i) {
        R_RenderPipeline* pipeline
          = ARENA_GET_TYPE(&_RenderPipelineArena, R_RenderPipeline, i);
        // compare config
        if (memcmp(&pipeline->config, &config, sizeof(R_MaterialConfig)) == 0) {
            return pipeline;
        }
    }

    // else create a new one
    R_RenderPipeline* rPipeline
      = ARENA_PUSH_ZERO_TYPE(&_RenderPipelineArena, R_RenderPipeline);
    u64 pipelineOffset = Arena::offsetOf(&_RenderPipelineArena, rPipeline);
    R_RenderPipeline::init(gctx, rPipeline, config, pipelineOffset);

    // TODO consolidate with hashmap?
    ASSERT(_RenderPipelineMap.find(rPipeline->rid) == _RenderPipelineMap.end());
    _RenderPipelineMap[rPipeline->rid] = pipelineOffset;

    return rPipeline;
}

R_RenderPipeline* Component_GetPipeline(R_ID rid)
{
    if (_RenderPipelineMap.find(rid) == _RenderPipelineMap.end()) return NULL;
    return (R_RenderPipeline*)Arena::get(&_RenderPipelineArena,
                                         _RenderPipelineMap[rid]);
}

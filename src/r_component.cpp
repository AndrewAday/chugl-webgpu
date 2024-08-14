#include "r_component.h"
#include "core/hashmap.h"
#include "geometry.h"
#include "graphics.h"
#include "shaders.h"

#include "core/file.h"
#include "core/log.h"

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
//
// set for materials to create pipelines for
static hashmap* materials_with_new_pso = NULL;

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

R_Scene* R_Transform::getScene(R_Transform* xform)
{
    // walk up parent chain until scene is found
    while (xform) {
        if (xform->type == SG_COMPONENT_SCENE) return (R_Scene*)xform;
        xform = Component_GetXform(xform->parentID);
    }

    return NULL;
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

    // remove child subgraph from scene render state
    R_Scene* scene = R_Transform::getScene(parent);
    R_Scene::removeSubgraphFromRenderState(scene, child);
}

void R_Transform::addChild(R_Transform* parent, R_Transform* child)
{
    if (R_Transform::isAncestor(child, parent)) {
        log_error("No cycles in scenegraph; cannot add parent as child of descendent");
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
    SG_ID* xformID = ARENA_PUSH_ZERO_TYPE(&parent->children, SG_ID);
    *xformID       = child->id;

    R_Transform::setStale(child, R_Transform_STALE_WORLD);

    // add child subgraph to scene render state
    R_Scene* scene = R_Transform::getScene(parent);
    R_Scene::addSubgraphToRenderState(scene, child);
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
    xform->_pos  = glm::vec3(M[3]);
    xform->_rot  = glm::quat_cast(M);
    xform->_sca  = glm::vec3(glm::length(M[0]), glm::length(M[1]), glm::length(M[2]));
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
static void _Transform_RebuildDescendants(R_Scene* scene, R_Transform* xform,
                                          const glm::mat4* parentWorld)
{
    // mark primitive as stale since world matrix will change
    if (xform->_geoID && xform->_matID) {
        R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID)->stale = true;
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
        _Transform_RebuildDescendants(scene, child, &xform->world);
    }
}

void R_Transform::rebuildMatrices(R_Scene* root, Arena* arena)
{
    // push root onto stack
    SG_ID* base = ARENA_PUSH_ZERO_TYPE(arena, SG_ID);
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
                for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
                    SG_ID* childID = ARENA_PUSH_ZERO_TYPE(arena, SG_ID);
                    *childID       = children[i];
                }
                break;
            }
            case R_Transform_STALE_WORLD:
            case R_Transform_STALE_LOCAL: {
                // get parent world matrix
                R_Transform* parent = Component_GetXform(xform->parentID);
                _Transform_RebuildDescendants(root, xform,
                                              parent ? &parent->world : &identityMat);
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
    R_Transform::rot(xform, xform->_rot * glm::angleAxis(deg, glm::normalize(axis)));
}

void R_Transform::rotateOnWorldAxis(R_Transform* xform, glm::vec3 axis, f32 deg)
{
    R_Transform::rot(xform, glm::angleAxis(deg, glm::normalize(axis)) * xform->_rot);
}

void R_Transform::print(R_Transform* xform)
{
    R_Transform::print(xform, 0);
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

u32 R_Geometry::indexCount(R_Geometry* geo)
{
    return geo->gpu_index_buffer.size / sizeof(u32);
}

u32 R_Geometry::vertexCount(R_Geometry* geo)
{
    if (geo->vertex_attribute_num_components[0] == 0) return 0;

    return geo->gpu_vertex_buffers[0].size
           / (sizeof(f32) * geo->vertex_attribute_num_components[0]);
}

// returns # of contiguous non-zero vertex attributes
u32 R_Geometry::vertexAttributeCount(R_Geometry* geo)
{
    for (int i = 0; i < ARRAY_LENGTH(geo->vertex_attribute_num_components); ++i) {
        if (geo->vertex_attribute_num_components[i] == 0) return i;
    }
    return ARRAY_LENGTH(geo->vertex_attribute_num_components);
}

void R_Geometry::buildFromVertices(GraphicsContext* gctx, R_Geometry* geo,
                                   Vertices* vertices)
{
    // TODO: optimization. Flag if tangents are already present
    // actually tangents should be built before being passed...
    Vertices::buildTangents(vertices);

    // write indices
    if (vertices->indicesCount > 0) {
        R_Geometry::setIndices(gctx, geo, vertices->indices, vertices->indicesCount);
    }

    // TODO probably refactor vertices to encode attrib layout (# components and
    // location)
    { // write vertices
        R_Geometry::setVertexAttribute(gctx, geo, 0, 3, Vertices::positions(vertices),
                                       vertices->vertexCount);

        R_Geometry::setVertexAttribute(gctx, geo, 1, 3, Vertices::normals(vertices),
                                       vertices->vertexCount);

        R_Geometry::setVertexAttribute(gctx, geo, 2, 2, Vertices::texcoords(vertices),
                                       vertices->vertexCount);

        R_Geometry::setVertexAttribute(gctx, geo, 3, 4, Vertices::tangents(vertices),
                                       vertices->vertexCount);
    }

    Vertices::free(vertices);
}

// data count is # of floats in data array NOT length / num_components
void R_Geometry::setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo,
                                    u32 location, u32 num_components, f32* data,
                                    u32 data_count)
{
    ASSERT(location >= 0
           && location < ARRAY_LENGTH(geo->vertex_attribute_num_components));

    ASSERT(data_count % num_components == 0);

    geo->vertex_attribute_num_components[location] = num_components;
    GPU_Buffer::write(gctx, &geo->gpu_vertex_buffers[location],
                      (WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst), data,
                      data_count * sizeof(f32));
}

void R_Geometry::setIndices(GraphicsContext* gctx, R_Geometry* geo, u32* indices,
                            u32 indices_count)
{
    GPU_Buffer::write(gctx, &geo->gpu_index_buffer,
                      (WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst), indices,
                      indices_count * sizeof(*indices));
}

bool R_Geometry::usesVertexPulling(R_Geometry* geo)
{
    for (int i = 0; i < ARRAY_LENGTH(geo->pull_buffers); ++i) {
        if (geo->pull_buffers[i].buf) return true;
    }
    return false;
}
void R_Geometry::rebuildPullBindGroup(GraphicsContext* gctx, R_Geometry* geo,
                                      WGPUBindGroupLayout layout)
{
    if (!geo->pull_bind_group_dirty) {
        return;
    }

    geo->pull_bind_group_dirty = false;

    WGPUBindGroupEntry entries[SG_GEOMETRY_MAX_VERTEX_PULL_BUFFERS];
    int num_entries = 0;
    for (u32 i = 0; i < SG_GEOMETRY_MAX_VERTEX_PULL_BUFFERS; i++) {
        if (geo->pull_buffers[i].size == 0) {
            continue;
        }

        WGPUBindGroupEntry* entry = &entries[num_entries++];
        entry->binding            = i;
        entry->buffer             = geo->pull_buffers[i].buf;
        entry->offset             = 0;
        entry->size               = geo->pull_buffers[i].size;
    }

    WGPUBindGroupDescriptor desc = {};
    desc.layout                  = layout;
    desc.entryCount              = num_entries;
    desc.entries                 = entries;

    WGPU_RELEASE_RESOURCE(BindGroup, geo->pull_bind_group);

    geo->pull_bind_group = wgpuDeviceCreateBindGroup(gctx->device, &desc);
    ASSERT(geo->pull_bind_group);
}

void R_Geometry::setPulledVertexAttribute(GraphicsContext* gctx, R_Geometry* geo,
                                          u32 location, void* data, size_t size_bytes)
{
    int prev_size  = geo->pull_buffers[location].size;
    bool recreated = GPU_Buffer::write(gctx, &geo->pull_buffers[location],
                                       WGPUBufferUsage_Storage, data, size_bytes);
    if (recreated || prev_size != size_bytes) {
        // need to update bindgroup size
        geo->pull_bind_group_dirty = true;
    }
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
// R_Material
// ============================================================================

void MaterialTextureView::init(MaterialTextureView* view)
{
    *view          = {};
    view->strength = 1.0f;
    view->scale[0] = 1.0f;
    view->scale[1] = 1.0f;
}

// static void _R_Material_Init(R_Material* mat)
//{
//     ASSERT(mat->id == 0);
//     ASSERT(mat->pipelineID == 0);
//     *mat       = {};
//     mat->id    = getNewComponentID();
//     mat->type  = SG_COMPONENT_MATERIAL;
//     mat->stale = true;
//
//     // initialize primitives arena
//     Arena::init(&mat->primitives, sizeof(Material_Primitive) * 8);
//
//     // initialize bind entry arenas
//     Arena::init(&mat->bindings, sizeof(R_Binding) * 8);
//     Arena::init(&mat->bindGroupEntries, sizeof(WGPUBindGroupEntry) * 8);
//     Arena::init(&mat->uniformData, 64);
// }

// void R_Material::init(GraphicsContext* gctx, R_Material* mat, R_MaterialConfig*
// config)
// {
//     _R_Material_Init(mat);

//     // copy config (TODO do we even need to save this? it's stored in the
//     // pipeline anyways)
//     mat->config = *config;

//     // get pipeline
//     R_RenderPipeline* pipeline = Component_GetPipeline(gctx, config);

//     // add material to pipeline
//     R_RenderPipeline::addMaterial(pipeline, mat);
//     ASSERT(mat->pipelineID != 0);

//     // init texture views
//     // MaterialTextureView::init(&mat->baseColorTextureView);
//     // MaterialTextureView::init(&mat->metallicRoughnessTextureView);
//     // MaterialTextureView::init(&mat->normalTextureView);

//     // init base color to white
//     // mat->baseColor = glm::vec4(1.0f);
// }

void R_Material::updatePSO(GraphicsContext* gctx, R_Material* mat,
                           SG_MaterialPipelineState* pso)
{
    mat->pso = *pso;
    hashmap_set(materials_with_new_pso, &mat->id);
    mat->pipeline_stale = true;
}

void R_Material::rebuildBindGroup(R_Material* mat, GraphicsContext* gctx,
                                  WGPUBindGroupLayout layout)
{
    if (mat->fresh_bind_group) return;
    mat->fresh_bind_group = true;

    // create bindgroups for all bindings
    WGPUBindGroupEntry new_bind_group_entries[SG_MATERIAL_MAX_UNIFORMS] = {};
    int bind_group_index                                                = 0;
    ASSERT(SG_MATERIAL_MAX_UNIFORMS == ARRAY_LENGTH(mat->bindings));

    for (u32 i = 0; i < SG_MATERIAL_MAX_UNIFORMS; ++i) {
        // _R_Material_BindGroupEntryFromBinding(gctx, mat, binding, entryDesc, i);
        R_Binding* binding = &mat->bindings[i];
        if (binding->type == R_BIND_EMPTY) continue;

        WGPUBindGroupEntry* bind_group_entry
          = &new_bind_group_entries[bind_group_index++];
        *bind_group_entry = {};

        bind_group_entry->binding = i; // binding location

        switch (binding->type) {
            case R_BIND_UNIFORM: {
                bind_group_entry->offset = sizeof(SG_MaterialUniformData) * i;
                bind_group_entry->size   = sizeof(SG_MaterialUniformData);
                bind_group_entry->buffer = mat->uniform_buffer.buf;
            } break;
            case R_BIND_STORAGE: {
                bind_group_entry->offset = 0;
                bind_group_entry->size   = binding->size;
                bind_group_entry->buffer = binding->as.storage_buffer.buf;
            } break;
            case R_BIND_SAMPLER: {
                bind_group_entry->sampler
                  = Graphics_GetSampler(gctx, binding->as.samplerConfig);
            } break;
            case R_BIND_TEXTURE_ID: {
                R_Texture* rTexture = Component_GetTexture(binding->as.textureID);
                bind_group_entry->textureView = rTexture->gpu_texture.view;
            } break;
            default: ASSERT(false);
        }
    }

    // release previous bindgroup
    WGPU_RELEASE_RESOURCE(BindGroup, mat->bind_group);

    // create new bindgroup
    WGPUBindGroupDescriptor bg_desc = {};
    bg_desc.layout                  = layout;
    bg_desc.entryCount              = bind_group_index;
    bg_desc.entries                 = new_bind_group_entries;
    mat->bind_group                 = wgpuDeviceCreateBindGroup(gctx->device, &bg_desc);
    ASSERT(mat->bind_group);
}

static SamplerConfig samplerConfigFromSGSampler(SG_Sampler sg_sampler)
{
    // TODO make SamplerConfig and SG_Sampler just use WGPU types
    SamplerConfig sampler = {};
    sampler.wrapU         = (SamplerWrapMode)sg_sampler.wrapU;
    sampler.wrapV         = (SamplerWrapMode)sg_sampler.wrapV;
    sampler.wrapW         = (SamplerWrapMode)sg_sampler.wrapW;
    sampler.filterMin     = (SamplerFilterMode)sg_sampler.filterMin;
    sampler.filterMag     = (SamplerFilterMode)sg_sampler.filterMag;
    sampler.filterMip     = (SamplerFilterMode)sg_sampler.filterMip;
    return sampler;
}

void R_Material::setSamplerBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                   SG_Sampler sampler)
{
    SamplerConfig sampler_config = samplerConfigFromSGSampler(sampler);
    R_Material::setBinding(gctx, mat, location, R_BIND_SAMPLER, &sampler_config,
                           sizeof(sampler_config));
}

void R_Material::setTextureBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                   SG_ID texture_id)
{
    R_Material::setBinding(gctx, mat, location, R_BIND_TEXTURE_ID, &texture_id,
                           sizeof(texture_id));
}

void R_Material::setBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                            R_BindType type, void* data, size_t bytes)
{
    R_Binding* binding = &mat->bindings[location];

    // rebuild binding logic
    if (binding->type != type) {
        // need to rebuild bind group, the binding entry at `location` has changed
        mat->fresh_bind_group = false;
    }
    // rebuild logic for storage bindings
    else if (binding->type == R_BIND_STORAGE && binding->size != bytes) {
        // for wgsl builtin arrayLength() to work, need to update the
        // corresponding BindGroupEntry size
        mat->fresh_bind_group = false;
    }
    // rebuild logic for samplers
    else if (type == R_BIND_SAMPLER) {
        ASSERT(bytes == sizeof(SamplerConfig));
        // if the sampler has changed, need to rebuild bind group
        if (memcmp(&binding->as.samplerConfig, data, bytes) != 0) {
            mat->fresh_bind_group = false;
        }
    }
    // rebuild logic for textures
    // TODO support change in texture type, size, etc
    else if (type == R_BIND_TEXTURE_ID) {
        // if the texture has changed, need to rebuild bind group
        if (binding->as.textureID != *(SG_ID*)data) {
            mat->fresh_bind_group = false;
        }
    }

    binding->type = type;
    binding->size = bytes;

    // create new binding
    switch (type) {
        case R_BIND_UNIFORM: {
            size_t offset = sizeof(SG_MaterialUniformData) * location;
            // write unfiform data to corresponding GPU location
            GPU_Buffer::write(gctx, &mat->uniform_buffer, WGPUBufferUsage_Uniform,
                              offset, data, bytes);
        } break;
        case R_BIND_TEXTURE_ID: {
            ASSERT(bytes == sizeof(SG_ID));
            binding->as.textureID = *(SG_ID*)data;
            // TODO refcount
            break;
        }
        // case R_BIND_TEXTURE_VIEW: {
        //     ASSERT(bytes == sizeof(WGPUTextureView));
        //     binding->as.textureView = *(WGPUTextureView*)data;
        //     break;
        // }
        case R_BIND_SAMPLER: {
            ASSERT(bytes == sizeof(SamplerConfig));
            binding->as.samplerConfig = *(SamplerConfig*)data;
        } break;
        case R_BIND_STORAGE: {
            GPU_Buffer::write(gctx, &binding->as.storage_buffer,
                              WGPUBufferUsage_Storage, data, bytes);
        } break;
        default:
            // if the new binding is also STORAGE reuse the memory, don't
            // free
            log_error("unsupported binding type %d", type);
            ASSERT(false);
            break;
    }
}

// assumes sampler is at location+1
void R_Material::setTextureAndSamplerBinding(R_Material* mat, u32 location,
                                             SG_ID textureID,
                                             WGPUTextureView defaultView)
{
    ASSERT(false);
#if 0
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
        R_Material::setBinding(mat, location + 1, R_BIND_SAMPLER, &defaultSampler,
                               sizeof(SamplerConfig));
    }
#endif
}

// ============================================================================
// R_Scene
// ============================================================================

void GeometryToXforms::rebuildBindGroup(GraphicsContext* gctx, R_Scene* scene,
                                        GeometryToXforms* g2x,
                                        WGPUBindGroupLayout layout, Arena* frame_arena)
{
    if (!g2x->stale) return;
    g2x->stale = false;

    // build new array of matrices on CPU
    u64 model_matrices_offset = frame_arena->curr;
    int matrixCount           = 0;

    int numInstances = ARENA_LENGTH(&g2x->xform_ids, SG_ID);
    SG_ID* xformIDs  = (SG_ID*)g2x->xform_ids.base;
    // delete and swap any destroyed xforms
    for (int i = 0; i < numInstances; ++i) {
        R_Transform* xform = Component_GetXform(xformIDs[i]);
        // remove NULL xforms and xforms that have been reassigned new mesh params
        // TODO: impl GMesh.geo() and GMesh.mat() to change geo and mat of mesh
        // - impl needs to set GeometryToXforms.stale = true
        // - but does NOT need to linear search the xformIDs arena. because lazy
        // deletion happens right here
        bool xform_destroyed = (xform == NULL);
        bool xform_changed_mesh
          = (xform->_geoID != g2x->key.geo_id || xform->_matID != g2x->key.mat_id);
        bool xform_detached_from_scene = (xform->scene_id != scene->id);
        if (xform_destroyed || xform_changed_mesh || xform_detached_from_scene) {
            // swap with last element
            xformIDs[i] = xformIDs[numInstances - 1];
            // pop last element
            Arena::pop(&g2x->xform_ids, sizeof(SG_ID));
            // decrement to reprocess this index
            --i;
            --numInstances;
            continue;
        }
        // assert his xform belongs to this material and geometry
        ASSERT(xform->_geoID == g2x->key.geo_id);
        ASSERT(xform->_matID == g2x->key.mat_id);
        // else add xform matrix to arena
        ++matrixCount; // TODO remove matrixCount variable if this all works
        // world matrix should already have been computed by now
        ASSERT(xform->_stale == R_Transform_STALE_NONE);
        *ARENA_PUSH_TYPE(frame_arena, glm::mat4) = xform->world;

        // TODO add normal matrix
    }
    // sanity check that we have the correct number of matrices
    ASSERT(matrixCount == numInstances);
    ASSERT(numInstances == ARENA_LENGTH(&g2x->xform_ids, SG_ID));

    u64 write_size = frame_arena->curr - model_matrices_offset;
    GPU_Buffer::write(gctx, &g2x->xform_storage_buffer, WGPUBufferUsage_Storage,
                      Arena::get(frame_arena, model_matrices_offset), write_size);

    // pop arena after copying data to GPU
    Arena::pop(frame_arena, write_size);
    ASSERT(model_matrices_offset == frame_arena->curr);

    // recreate bindgroup
    WGPUBindGroupEntry entry = {};
    entry.binding            = 0;
    entry.buffer             = g2x->xform_storage_buffer.buf;
    entry.offset             = 0;
    entry.size               = g2x->xform_storage_buffer.size;

    WGPUBindGroupDescriptor desc = {};
    desc.layout                  = layout;
    desc.entryCount              = 1;
    desc.entries                 = &entry;

    WGPU_RELEASE_RESOURCE(BindGroup, g2x->xform_bind_group);

    g2x->xform_bind_group = wgpuDeviceCreateBindGroup(gctx->device, &desc);
    ASSERT(g2x->xform_bind_group);
}

void R_Scene::removeSubgraphFromRenderState(R_Scene* scene, R_Transform* root)
{
    ASSERT(scene->id == root->scene_id);

    if (!scene) return;

    // find all meshes in child subgraph
    static Arena arena{};
    defer(Arena::clear(&arena));

    SG_ID* sg_id = ARENA_PUSH_TYPE(&arena, SG_ID);
    *sg_id       = root->id;
    while (ARENA_LENGTH(&arena, SG_ID)) {
        SG_ID xformID = *ARENA_GET_LAST_TYPE(&arena, SG_ID);
        ARENA_POP_TYPE(&arena, SG_ID);
        R_Transform* xform = Component_GetXform(xformID);

        // add children to queue
        for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
            *ARENA_PUSH_ZERO_TYPE(&arena, SG_ID)
              = *ARENA_GET_TYPE(&xform->children, SG_ID, i);
        }

        // remove scene from xform
        ASSERT(xform->scene_id == scene->id);
        xform->scene_id = 0;

        if (xform->type == SG_COMPONENT_MESH) {
            // get xforms from geometry
            GeometryToXforms* g2x
              = R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID);
            ASSERT(g2x); // GMesh was added, so geometry should exist

            // don't need to remove xform from geometry
            // just mark the g2x entry as stale; will be lazily deleted in
            // GeometryToXforms::rebuildBindGroup();
            g2x->stale = true;
            ASSERT(Arena::containsItem(&g2x->xform_ids, &xform->id, sizeof(xform->id)));

            // TODO: deletions must cascade upwards to geo-->material-->pipeline
            // we can only remove an SG_ID from a higher level IF that SG_ID
            // at the lower level has an empty arena

            // can implement this later. for now deletions will leave a bunch of empty
            // arenas in the 3 hashmaps.
            // better yet: batch all deletions and prune the render state in one go
            // in a separate function like GG.gc()
        }
    }
}

void R_Scene::addSubgraphToRenderState(R_Scene* scene, R_Transform* root)
{
    ASSERT(scene == R_Transform::getScene(root));

    if (!scene) return;

    // find all meshes in child subgraph
    static Arena arena{};
    defer(Arena::clear(&arena));

    *ARENA_PUSH_TYPE(&arena, SG_ID) = root->id;
    while (ARENA_LENGTH(&arena, SG_ID)) {
        R_Transform* xform = Component_GetXform(*ARENA_GET_LAST_TYPE(&arena, SG_ID));
        ARENA_POP_TYPE(&arena, SG_ID);

        // add children to queue
        for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
            *ARENA_PUSH_ZERO_TYPE(&arena, SG_ID)
              = *ARENA_GET_TYPE(&xform->children, SG_ID, i);
        }

        // add scene to transform state
        ASSERT(xform->scene_id != scene->id);
        xform->scene_id = scene->id;

        if (xform->type == SG_COMPONENT_MESH) {
            bool add_m2g_and_p2m_entries = false;

            // try adding to bottom level GeometryToXform
            // if its not present, build up entire chain:
            // g2x (geo, mat) --> xforms
            // m2g mat --> geo
            // p2g pso --> mat
            // maintain invariant that if a (geo, mat) is present in g2x,
            // then all higher order entries are also present.
            // if g2x entry is not found, create it and all corresponding.
            // invariant holds assuming the deletion in removeSubgraphFromRenderState()
            // does not delete entire hashmap entries, ONLY pops individual SG_IDs from
            // the entry arena
            // that is, if a <geo_id, mat_id> entry is present in g2m hashmap,
            // even if it has 0 xform ids, all upwards mat_id --> geo_ids and
            // pipeline_id --> mat_ids entries also exist
            // this avoids us having to do O(n) search on each xform added/removed
            // from scenegraph render state

            GeometryToXforms* g2x
              = R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID);
            if (g2x == NULL) {
                // add to hashmap, need to add upward entries material-->geo and
                // pipeline-->mat
                GeometryToXforms new_g2x = {};
                new_g2x.key.geo_id       = xform->_geoID;
                new_g2x.key.mat_id       = xform->_matID;
                Arena::init(&new_g2x.xform_ids, sizeof(SG_ID) * 8);
                hashmap_set(scene->geo_to_xform, &new_g2x);
                g2x = R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID);
                add_m2g_and_p2m_entries = true;
            }

            ASSERT(g2x->key.geo_id == xform->_geoID);
            ASSERT(g2x->key.mat_id == xform->_matID);

            // check if xform already exists in geometry
            ASSERT(
              !Arena::containsItem(&g2x->xform_ids, &xform->id, sizeof(xform->id)));

            // add xform to geometry
            *ARENA_PUSH_ZERO_TYPE(&g2x->xform_ids, SG_ID) = xform->id;
            g2x->stale = true; // need to rebuild bindgroup

            if (add_m2g_and_p2m_entries) {
                // get geometry from material
                MaterialToGeometry* m2g = (MaterialToGeometry*)hashmap_get(
                  scene->material_to_geo, &xform->_matID);

                if (m2g == NULL) {
                    MaterialToGeometry new_m2g = {};
                    new_m2g.material_id        = xform->_matID;
                    Arena::init(&new_m2g.geo_ids, sizeof(SG_ID) * 8);
                    hashmap_set(scene->material_to_geo, &new_m2g);
                    m2g = (MaterialToGeometry*)hashmap_get(scene->material_to_geo,
                                                           &xform->_matID);
                }

                // invariant says entry should not have been added
                ASSERT(!Arena::containsItem(&m2g->geo_ids, &xform->_matID,
                                            sizeof(xform->_matID)));
                *ARENA_PUSH_ZERO_TYPE(&m2g->geo_ids, SG_ID) = xform->_geoID;
            }
        }
    }
}

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

    // initialize arenas
    int seed = time(NULL);
    r_scene->material_to_geo
      = hashmap_new(sizeof(MaterialToGeometry), 0, seed, seed, MaterialToGeometry::hash,
                    MaterialToGeometry::compare, MaterialToGeometry::free, NULL);
    r_scene->geo_to_xform
      = hashmap_new(sizeof(GeometryToXforms), 0, seed, seed, GeometryToXforms::hash,
                    GeometryToXforms::compare, GeometryToXforms::free, NULL);

    // initialize children array for 8 children
    Arena::init(&r_scene->children, sizeof(SG_ID) * 8);
}

// ============================================================================
// Render Pipeline Definitions
// ============================================================================

GPU_Buffer R_RenderPipeline::frame_uniform_buffer = {};

void R_RenderPipeline::init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
                            const SG_MaterialPipelineState* config)
{
    ASSERT(pipeline->gpu_pipeline == NULL);
    ASSERT(pipeline->rid == 0);

    pipeline->rid = getNewRID();
    ASSERT(pipeline->rid < 0);

    pipeline->pso = *config;

    // const char* vertShaderCode = NULL;
    // const char* fragShaderCode = NULL;
    // switch (config->material_type) {
    //     case SG_MATERIAL_PBR: {
    //         vertShaderCode = shaderCode;
    //         fragShaderCode = shaderCode;
    //         break;
    //     }
    //     default: ASSERT(false && "unsupported shader type");
    // }

    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology           = config->primitive_topology;
    primitiveState.stripIndexFormat
      = (primitiveState.topology == WGPUPrimitiveTopology_TriangleStrip
         || primitiveState.topology == WGPUPrimitiveTopology_LineStrip) ?
          WGPUIndexFormat_Uint32 :
          WGPUIndexFormat_Undefined;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    primitiveState.cullMode  = config->cull_mode;

    // TODO transparency (dissallow partial transparency, see if fragment
    // discard writes to the depth buffer)
    WGPUBlendState blendState = G_createBlendState(true);

    WGPUColorTargetState colorTargetState = {};
    colorTargetState.format               = gctx->swapChainFormat;
    colorTargetState.blend                = &blendState;
    colorTargetState.writeMask            = WGPUColorWriteMask_All;

    WGPUDepthStencilState depth_stencil_state
      = G_createDepthStencilState(WGPUTextureFormat_Depth24PlusStencil8, true);

    // Setup shader module
    R_Shader* shader = Component_GetShader(config->sg_shader_id);
    ASSERT(shader);

    VertexBufferLayout vertexBufferLayout = {};
    VertexBufferLayout::init(&vertexBufferLayout, ARRAY_LENGTH(shader->vertex_layout),
                             (u32*)shader->vertex_layout);

    // vertex state
    WGPUVertexState vertexState = {};
    vertexState.bufferCount     = vertexBufferLayout.attribute_count;
    vertexState.buffers         = vertexBufferLayout.layouts;
    vertexState.module          = shader->vertex_shader_module;
    vertexState.entryPoint      = VS_ENTRY_POINT;

    // fragment state
    WGPUFragmentState fragmentState = {};
    fragmentState.module            = shader->fragment_shader_module;
    fragmentState.entryPoint        = FS_ENTRY_POINT;
    fragmentState.targetCount       = 1;
    fragmentState.targets           = &colorTargetState;

    // multisample state
    WGPUMultisampleState multisampleState   = {};
    multisampleState.count                  = 1;
    multisampleState.mask                   = 0xFFFFFFFF;
    multisampleState.alphaToCoverageEnabled = false;

    char pipeline_label[64] = {};
    snprintf(pipeline_label, sizeof(pipeline_label), "RenderPipeline %lld %s",
             (i64)shader->id, shader->name.c_str());
    WGPURenderPipelineDescriptor pipeline_desc = {};
    pipeline_desc.label                        = pipeline_label;
    pipeline_desc.layout                       = NULL; // Using layout: auto
    pipeline_desc.primitive                    = primitiveState;
    pipeline_desc.vertex                       = vertexState;
    pipeline_desc.fragment                     = &fragmentState;
    pipeline_desc.depthStencil                 = &depth_stencil_state;
    pipeline_desc.multisample                  = multisampleState;

    pipeline->gpu_pipeline
      = wgpuDeviceCreateRenderPipeline(gctx->device, &pipeline_desc);
    ASSERT(pipeline->gpu_pipeline);

    { // create per-frame bindgroup
        // implicit layouts cannot share bindgroups, so need to create one
        // per render pipeline

        // initialize shared frame buffer
        if (!R_RenderPipeline::frame_uniform_buffer.buf) {
            FrameUniforms frame_uniforms = {};
            GPU_Buffer::write(gctx, &R_RenderPipeline::frame_uniform_buffer,
                              WGPUBufferUsage_Uniform, &frame_uniforms,
                              sizeof(frame_uniforms));
        }

        // create bind group entry
        WGPUBindGroupEntry frame_group_entry = {};
        frame_group_entry                    = {};
        frame_group_entry.binding            = 0;
        frame_group_entry.buffer             = pipeline->frame_uniform_buffer.buf;
        frame_group_entry.size               = pipeline->frame_uniform_buffer.size;

        // create bind group
        WGPUBindGroupDescriptor frameGroupDesc;
        frameGroupDesc        = {};
        frameGroupDesc.layout = wgpuRenderPipelineGetBindGroupLayout(
          pipeline->gpu_pipeline, PER_FRAME_GROUP);
        frameGroupDesc.entries    = &frame_group_entry;
        frameGroupDesc.entryCount = 1;

        // layout:auto requires a bind group per pipeline
        pipeline->frame_group
          = wgpuDeviceCreateBindGroup(gctx->device, &frameGroupDesc);
        ASSERT(pipeline->frame_group);
    }

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
- cons: requires a second pass over all materials, which is not cache
efficient

Option 2: check the material WHEN doing the render pass over all
RenderPipeline. Same logic, GetRenderPipelineConfig() and if its different,
add to correct pipeline.
- cons: if the material gets added to a pipeline that was ALREADY processed,
it will be missed and only rendered on the NEXT frame. (that might be fine)

Going with option 2 for now

*/

void R_RenderPipeline::addMaterial(R_RenderPipeline* pipeline, R_Material* material)
{
    if (material->pipelineID == pipeline->rid) {
        ASSERT(ARENA_CONTAINS(&pipeline->materialIDs, material->id));
        return;
    }

    // remove material from existing pipeline
    // Lazy deletion. during render loop, pipeline checks if the material
    // ID matches this current pipeline. If not, we know we've switched.
    // the render pipeline then removes the material from its list of
    // material IDs. This avoids the cost of doing a linear search over all
    // materials for deletion. We postpone the deletion until we're already
    // iterating over the materials anyways

    // add material to new pipeline
    ASSERT(!ARENA_CONTAINS(&pipeline->materialIDs, material->id));
    *ARENA_PUSH_TYPE(&pipeline->materialIDs, SG_ID) = material->id;
    material->pipelineID                            = pipeline->rid;
}

size_t R_RenderPipeline::numMaterials(R_RenderPipeline* pipeline)
{
    return ARENA_LENGTH(&pipeline->materialIDs, SG_ID);
}

// While iterating, will lazy delete NULL material IDs and materials
// whose config doesn't match the pipeline (e.g. material has been assigned
// elsewhere) swap NULL with last element and pop
// Can do away with this lazy deletion if we augment the SG_ID arena with a
// hashmap for O(1) material ID lookup and deletion
bool R_RenderPipeline::materialIter(R_RenderPipeline* pipeline, size_t* indexPtr,
                                    R_Material** material)
{
    size_t numMaterials = R_RenderPipeline::numMaterials(pipeline);

    if (*indexPtr >= numMaterials) {
        *material = NULL;
        return false;
    }

    SG_ID* materialID = ARENA_GET_TYPE(&pipeline->materialIDs, SG_ID, *indexPtr);
    *material         = Component_GetMaterial(*materialID);

    // if null or reassigned to different pipeline, swap with last element
    // and try again
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
static Arena shaderArena;
static Arena materialArena;
static Arena textureArena;
static Arena _RenderPipelineArena;

// default textures
static Texture opaqueWhitePixel      = {};
static Texture transparentBlackPixel = {};
static Texture defaultNormalPixel    = {};

// maps from id --> offset
static hashmap* r_locator                 = NULL;
static hashmap* render_pipeline_pso_table = NULL; // lookup by pso
static hashmap* _RenderPipelineMap;               // lookup by rid

struct R_Location {
    SG_ID id;     // key
    u64 offset;   // value (byte offset into arena)
    Arena* arena; // where to find
};

struct RenderPipelinePSOTableItem {
    SG_MaterialPipelineState pso; // key
    u64 pipeline_offset;          // item

    static int compare(const void* a, const void* b, void* udata)
    {
        RenderPipelinePSOTableItem* ga = (RenderPipelinePSOTableItem*)a;
        RenderPipelinePSOTableItem* gb = (RenderPipelinePSOTableItem*)b;
        return memcmp(&ga->pso, &gb->pso, sizeof(ga->pso));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        RenderPipelinePSOTableItem* g2x = (RenderPipelinePSOTableItem*)item;
        return hashmap_xxhash3(&g2x->pso, sizeof(g2x->pso), seed0, seed1);
    }
};

struct RenderPipelineIDTableItem {
    R_ID id;             // key
    u64 pipeline_offset; // item

    static int compare(const void* a, const void* b, void* udata)
    {
        RenderPipelineIDTableItem* ga = (RenderPipelineIDTableItem*)a;
        RenderPipelineIDTableItem* gb = (RenderPipelineIDTableItem*)b;
        return memcmp(&ga->id, &gb->id, sizeof(ga->id));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        RenderPipelineIDTableItem* g2x = (RenderPipelineIDTableItem*)item;
        return hashmap_xxhash3(&g2x->id, sizeof(g2x->id), seed0, seed1);
    }
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

static int compareSGIDs(const void* a, const void* b, void* udata)
{
    return *(SG_ID*)a - *(SG_ID*)b;
}

static uint64_t hashSGID(const void* item, uint64_t seed0, uint64_t seed1)
{
    return hashmap_xxhash3(item, sizeof(SG_ID), seed0, seed1);
}

void Component_Init(GraphicsContext* gctx)
{
    // initialize arena memory
    Arena::init(&xformArena, sizeof(R_Transform) * 128);
    Arena::init(&sceneArena, sizeof(R_Scene) * 128);
    Arena::init(&geoArena, sizeof(R_Geometry) * 128);
    Arena::init(&shaderArena, sizeof(R_Shader) * 64);
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
    render_pipeline_pso_table
      = hashmap_new(sizeof(RenderPipelinePSOTableItem), 0, seed, seed,
                    RenderPipelinePSOTableItem::hash,
                    RenderPipelinePSOTableItem::compare, NULL, NULL);

    _RenderPipelineMap = hashmap_new(sizeof(RenderPipelineIDTableItem), 0, seed, seed,
                                     RenderPipelineIDTableItem::hash,
                                     RenderPipelineIDTableItem::compare, NULL, NULL);
    materials_with_new_pso
      = hashmap_new(sizeof(SG_ID), 0, seed, seed, hashSGID, compareSGIDs, NULL, NULL);
}

void Component_Free()
{
    // TODO: should we also free the individual components?

    // free arena memory
    Arena::free(&xformArena);
    Arena::free(&sceneArena);
    Arena::free(&geoArena);
    Arena::free(&shaderArena);
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
    R_Transform* xform = ARENA_PUSH_ZERO_TYPE(&xformArena, R_Transform);
    R_Transform::init(xform);

    ASSERT(xform->id != 0);                        // ensure id is set
    ASSERT(xform->type == SG_COMPONENT_TRANSFORM); // ensure type is set

    // store offset
    R_Location loc = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd)
{
    R_Transform* xform = ARENA_PUSH_ZERO_TYPE(&xformArena, R_Transform);
    R_Transform::initFromSG(xform, cmd);

    ASSERT(xform->id != 0);                        // ensure id is set
    ASSERT(xform->type == SG_COMPONENT_TRANSFORM); // ensure type is set

    // store offset
    R_Location loc = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Transform* Component_CreateMesh(SG_Command_Mesh_Create* cmd)
{
    R_Transform* xform = ARENA_PUSH_ZERO_TYPE(&xformArena, R_Transform);

    {
        xform->id         = cmd->mesh_id;
        xform->type       = SG_COMPONENT_MESH;
        xform->xform_type = R_TRANSFORM_MESH;

        xform->_pos = glm::vec3(0.0f);
        xform->_rot = QUAT_IDENTITY;
        xform->_sca = glm::vec3(1.0f);

        xform->world  = MAT_IDENTITY;
        xform->local  = MAT_IDENTITY;
        xform->normal = MAT_IDENTITY;
        xform->_stale = R_Transform_STALE_LOCAL;

        xform->_geoID = cmd->geo_id;
        xform->_matID = cmd->mat_id;

        xform->parentID = 0;
        // initialize children array for 8 children
        Arena::init(&xform->children, sizeof(SG_ID) * 8);
    }

    // store offset
    R_Location loc = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Scene* Component_CreateScene(SG_Command_SceneCreate* cmd)
{
    Arena* arena     = &sceneArena;
    R_Scene* r_scene = ARENA_PUSH_ZERO_TYPE(arena, R_Scene);
    R_Scene::initFromSG(r_scene, cmd);

    ASSERT(r_scene->id != 0);                    // ensure id is set
    ASSERT(r_scene->type == SG_COMPONENT_SCENE); // ensure type is set

    // store offset
    R_Location loc     = { r_scene->id, Arena::offsetOf(arena, r_scene), arena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return r_scene;
}

R_Geometry* Component_CreateGeometry()
{
    R_Geometry* geo = ARENA_PUSH_ZERO_TYPE(&geoArena, R_Geometry);
    R_Geometry::init(geo);

    ASSERT(geo->id != 0);                       // ensure id is set
    ASSERT(geo->type == SG_COMPONENT_GEOMETRY); // ensure type is set

    // store offset
    R_Location loc     = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return geo;
}

R_Geometry* Component_CreateGeometry(GraphicsContext* gctx, SG_Command_GeoCreate* cmd)
{
    R_Geometry* geo = ARENA_PUSH_ZERO_TYPE(&geoArena, R_Geometry);

    geo->id           = cmd->sg_id;
    geo->type         = SG_COMPONENT_GEOMETRY;
    geo->vertex_count = -1; // -1 means draw all vertices

    // for now not storing geo_type (cube, sphere, custom etc.)
    // we only store the GPU vertex data, and don't care about semantics

    // store offset
    R_Location loc     = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return geo;
}

R_Shader* Component_CreateShader(GraphicsContext* gctx, SG_Command_ShaderCreate* cmd)
{
    R_Shader* shader = ARENA_PUSH_ZERO_TYPE(&shaderArena, R_Shader);

    shader->id   = cmd->sg_id;
    shader->type = SG_COMPONENT_SHADER;

    // shader member vars
    const char* vertex_string
      = (const char*)CQ_ReadCommandGetOffset(cmd->vertex_string_offset);
    const char* fragment_string
      = (const char*)CQ_ReadCommandGetOffset(cmd->fragment_string_offset);
    const char* vertex_filepath
      = (const char*)CQ_ReadCommandGetOffset(cmd->vertex_filepath_offset);
    const char* fragment_filepath
      = (const char*)CQ_ReadCommandGetOffset(cmd->fragment_filepath_offset);

    ASSERT(sizeof(cmd->vertex_layout) == sizeof(shader->vertex_layout));
    R_Shader::init(gctx, shader, vertex_string, vertex_filepath, fragment_string,
                   fragment_filepath, cmd->vertex_layout);

    // store offset
    R_Location loc
      = { shader->id, Arena::offsetOf(&shaderArena, shader), &shaderArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return shader;
}

// only called by renderer tester
// R_Material* Component_CreateMaterial(GraphicsContext* gctx,
// R_MaterialConfig* config)
// {
//     R_Material* mat = ARENA_PUSH_ZERO_TYPE(&materialArena, R_Material);
//     R_Material::init(gctx, mat, config);

//     ASSERT(mat->id != 0);                       // ensure id is set
//     ASSERT(mat->type == SG_COMPONENT_MATERIAL); // ensure type is set

//     // store offset
//     R_Location loc = { mat->id, Arena::offsetOf(&materialArena, mat),
//     &materialArena
//     }; const void* result = hashmap_set(r_locator, &loc); ASSERT(result
//     == NULL);
//     // ensure id is unique

//     return mat;
// }

// TODO: refactor so that R_Component doesn't know about SG_Command
R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd)
{
    R_Material* mat = ARENA_PUSH_ZERO_TYPE(&materialArena, R_Material);

    // initialize
    {
        *mat      = {};
        mat->id   = cmd->sg_id;
        mat->type = SG_COMPONENT_MATERIAL;

        GPU_Buffer::init(gctx, &mat->uniform_buffer, WGPUBufferUsage_Uniform,
                         sizeof(SG_MaterialUniformData) * ARRAY_LENGTH(mat->bindings));

        // build config
        mat->pso = cmd->pso;

        // defer pipeline (lazily created)
        const void* result = hashmap_set(materials_with_new_pso, &mat->id);
        ASSERT(result == NULL); // ensure id is unique
    }

    // set material params/uniforms
    // TODO this only works for PBR for now
    // {
    //     // TODO maybe consolidate SG_MaterialParams and R_MaterialParams
    //     // make the SG_MaterialParams struct alignment work for webgpu
    //     // uniforms
    //     MaterialUniforms pbr_uniforms  = {};
    //     SG_Material_PBR_Params* params = &cmd->params.pbr;
    //     pbr_uniforms.baseColor         = params->baseColor;
    //     pbr_uniforms.emissiveFactor    = params->emissiveFactor;
    //     pbr_uniforms.metallic          = params->metallic;
    //     pbr_uniforms.roughness         = params->roughness;
    //     pbr_uniforms.normalFactor      = params->normalFactor;
    //     pbr_uniforms.aoFactor          = params->aoFactor;

    //     R_Material::setBinding(mat, 0, R_BIND_UNIFORM, &pbr_uniforms,
    //                            sizeof(pbr_uniforms));

    //     R_Material::setTextureAndSamplerBinding(mat, 1, 0,
    //     opaqueWhitePixel.view);

    //     R_Material::setTextureAndSamplerBinding(mat, 3, 0,
    //     defaultNormalPixel.view);

    //     R_Material::setTextureAndSamplerBinding(mat, 5, 0,
    //     opaqueWhitePixel.view);

    //     R_Material::setTextureAndSamplerBinding(mat, 7, 0,
    //     opaqueWhitePixel.view);

    //     R_Material::setTextureAndSamplerBinding(mat, 9, 0,
    //     transparentBlackPixel.view);
    // }

    // store offset
    R_Location loc = { mat->id, Arena::offsetOf(&materialArena, mat), &materialArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return mat;
}

// called by renderer after flushing all commands
void Material_batchUpdatePipelines(GraphicsContext* gctx, SG_ID scene_id)
{
    R_Scene* main_scene = Component_GetScene(scene_id);

    // TODO:
    // handle xforms changing geo/mat by marking g2m as stale
    size_t index = 0;
    SG_ID* sg_id;
    while (hashmap_iter(materials_with_new_pso, &index, (void**)&sg_id)) {
        R_Material* mat            = Component_GetMaterial(*sg_id);
        R_RenderPipeline* pipeline = Component_GetPipeline(gctx, &mat->pso);

        ASSERT(mat->pipeline_stale);
        mat->pipeline_stale = false;

        // add material to pipeline
        R_RenderPipeline::addMaterial(pipeline, mat);
        ASSERT(mat->pipelineID == pipeline->rid);
    }

    // clear hashmap
    hashmap_clear(materials_with_new_pso, true);
}

R_Texture* Component_CreateTexture()
{
    R_Texture* texture = ARENA_PUSH_ZERO_TYPE(&textureArena, R_Texture);
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

R_Texture* Component_CreateTexture(GraphicsContext* gctx, SG_Command_TextureCreate* cmd)
{
    R_Texture* tex = ARENA_PUSH_ZERO_TYPE(&textureArena, R_Texture);

    tex->id   = cmd->sg_id;
    tex->type = SG_COMPONENT_TEXTURE;

    // TODO set texture attributes?

    // store offset
    R_Location loc = { tex->id, Arena::offsetOf(&textureArena, tex), &textureArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return tex;
}

R_Component* Component_GetComponent(SG_ID id)
{
    R_Location loc     = { id, 0, NULL };
    R_Location* result = (R_Location*)hashmap_get(r_locator, &loc);
    return result ? (R_Component*)Arena::get(result->arena, result->offset) : NULL;
}

R_Transform* Component_GetXform(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    if (comp) {
        ASSERT(comp->type == SG_COMPONENT_TRANSFORM || comp->type == SG_COMPONENT_SCENE
               || comp->type == SG_COMPONENT_MESH);
    }
    return (R_Transform*)comp;
}

R_Scene* Component_GetScene(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_SCENE);
    return (R_Scene*)comp;
}

R_Geometry* Component_GetGeometry(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_GEOMETRY);
    return (R_Geometry*)comp;
}

R_Shader* Component_GetShader(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_SHADER);
    return (R_Shader*)comp;
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
    *renderPipeline = ARENA_GET_TYPE(&_RenderPipelineArena, R_RenderPipeline, *i);
    ++(*i);
    return true;
}

int Component_RenderPipelineCount()
{
    return ARENA_LENGTH(&_RenderPipelineArena, R_RenderPipeline);
}

R_RenderPipeline* Component_GetPipeline(GraphicsContext* gctx,
                                        SG_MaterialPipelineState* pso)
{
    RenderPipelinePSOTableItem* rp_item
      = (RenderPipelinePSOTableItem*)hashmap_get(render_pipeline_pso_table, pso);
    if (rp_item)
        return (R_RenderPipeline*)Arena::get(&_RenderPipelineArena,
                                             rp_item->pipeline_offset);

    // else create a new one
    R_RenderPipeline* rPipeline
      = ARENA_PUSH_ZERO_TYPE(&_RenderPipelineArena, R_RenderPipeline);
    u64 pipelineOffset = Arena::offsetOf(&_RenderPipelineArena, rPipeline);
    R_RenderPipeline::init(gctx, rPipeline, pso);

    ASSERT(!hashmap_get(_RenderPipelineMap, &rPipeline->rid))
    RenderPipelineIDTableItem new_ri_item = { rPipeline->rid, pipelineOffset };
    hashmap_set(_RenderPipelineMap, &new_ri_item);

    RenderPipelinePSOTableItem new_rp_item = { *pso, pipelineOffset };
    hashmap_set(render_pipeline_pso_table, &new_rp_item);

    return rPipeline;
}

R_RenderPipeline* Component_GetPipeline(R_ID rid)
{
    return (R_RenderPipeline*)hashmap_get(_RenderPipelineMap, &rid);
}

// =============================================================================
// R_Shader
// =============================================================================

void R_Shader::init(GraphicsContext* gctx, R_Shader* shader, const char* vertex_string,
                    const char* vertex_filepath, const char* fragment_string,
                    const char* fragment_filepath, int* vertex_layout)
{
    char vertex_shader_label[32] = {};
    snprintf(vertex_shader_label, sizeof(vertex_shader_label), "vertex shader %d",
             (int)shader->id);
    char fragment_shader_label[32] = {};
    snprintf(fragment_shader_label, sizeof(fragment_shader_label), "fragment shader %d",
             (int)shader->id);

    if (vertex_string && strlen(vertex_string) > 0) {
        shader->vertex_shader_module
          = G_createShaderModule(gctx, vertex_string, vertex_shader_label);
    } else if (vertex_filepath && strlen(vertex_filepath) > 0) {
        // read entire file contents
        FileReadResult vertex_file = File_read(vertex_filepath, true);
        if (vertex_file.data_owned) {
            shader->vertex_shader_module = G_createShaderModule(
              gctx, (const char*)vertex_file.data_owned, vertex_shader_label);
            FREE(vertex_file.data_owned);
        } else {
            log_error("failed to read vertex shader file %s", vertex_filepath);
        }
    } else {
        ASSERT(false);
    }

    if (fragment_string && strlen(fragment_string) > 0) {
        shader->fragment_shader_module
          = G_createShaderModule(gctx, fragment_string, fragment_shader_label);
    } else if (fragment_filepath && strlen(fragment_filepath) > 0) {
        // read entire file contents
        FileReadResult fragment_file = File_read(fragment_filepath, true);
        if (fragment_file.data_owned) {
            shader->fragment_shader_module = G_createShaderModule(
              gctx, (const char*)fragment_file.data_owned, fragment_shader_label);
            FREE(fragment_file.data_owned);
        } else {
            log_error("failed to read fragment shader file %s", fragment_filepath);
        }
    } else {
        ASSERT(false);
    }

    // copy vertex layout
    memcpy(shader->vertex_layout, vertex_layout, sizeof(shader->vertex_layout));
}

void R_Shader::free(R_Shader* shader)
{
    WGPU_RELEASE_RESOURCE(ShaderModule, shader->vertex_shader_module);
    WGPU_RELEASE_RESOURCE(ShaderModule, shader->fragment_shader_module);
}

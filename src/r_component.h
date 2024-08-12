#pragma once

#include "graphics.h"
#include "sg_command.h"
#include "sg_component.h"

#include "core/macros.h"
#include "core/memory.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

// =============================================================================
// scenegraph data structures
// =============================================================================

struct Vertices;
struct R_Material;
struct R_Scene;
struct hashmap;

typedef i64 R_ID; // negative for R_Components NOT mapped to SG_Components

struct R_Component {
    SG_ID id; // SG_Component this R_Component is mapped to
    SG_ComponentType type;
    std::string name; // TODO move off std::string
};

// priority hiearchy for staleness
enum R_Transform_Staleness {
    R_Transform_STALE_NONE = 0,

    R_Transform_STALE_DESCENDENTS, // at least 1 descendent must recompute
                                   // world matrix

    R_Transform_STALE_WORLD, // world matrix of self and all descendents must
                             // be recomputed

    R_Transform_STALE_LOCAL, // local matrix of self must be recomputed,
                             // AND world matrix of self and all descendents
                             // must be recomputed
    R_Transform_STALE_COUNT
};

enum R_Transform_Type : u8 {
    R_TRANSFORM_NONE = 0,
    R_TRANSFORM_MESH,
    R_TRANSFORM_CAMERA,
    R_TRANSFORM_LIGHT,
    R_TRANSFORM_COUNT
};

struct R_Transform : public R_Component {
    // staleness flag has priority hiearchy, don't set directly!
    // instead use setStale()
    R_Transform_Staleness _stale;
    R_Transform_Type xform_type = R_TRANSFORM_NONE;

    // transform
    // don't update directly, otherwise staleness will be incorrect
    // use pos(), rot(), sca() instead
    glm::vec3 _pos;
    glm::quat _rot;
    glm::vec3 _sca;

    // world matrix (TODO cache)
    glm::mat4 world;
    glm::mat4 local;
    glm::mat4 normal; // normal matrix

    SG_ID parentID;
    Arena children; // stores list of SG_IDs

    // don't modify directly; use R_Material::addPrimitve() instead
    // Possibly separate this into R_Mesh / R_Camera / R_Light
    // for now holding all type data in the R_Transform struct
    // (maybe middle ground is to use a union { R_Mesh, R_Camera, R_Light })
    SG_ID _geoID;
    SG_ID _matID;

    SG_ID scene_id; // the scene this transform belongs to

    static void init(R_Transform* transform);
    static void initFromSG(R_Transform* r_xform, SG_Command_CreateXform* cmd);

    static void setStale(R_Transform* xform, R_Transform_Staleness stale);

    static glm::mat4 localMatrix(R_Transform* xform);

    /// @brief decompose matrix into transform data
    static void setXformFromMatrix(R_Transform* xform, const glm::mat4& M);

    static void setXform(R_Transform* xform, const glm::vec3& pos, const glm::quat& rot,
                         const glm::vec3& sca);
    static void pos(R_Transform* xform, const glm::vec3& pos);
    static void rot(R_Transform* xform, const glm::quat& rot);
    static void sca(R_Transform* xform, const glm::vec3& sca);

    // updates all local/world matrices in the scenegraph
    static void rebuildMatrices(R_Scene* root, Arena* arena);

    // Scenegraph relationships ----------------------------------------------
    // returns if ancestor is somewhere in the parent chain of descendent,
    // including descendent itself
    static bool isAncestor(R_Transform* ancestor, R_Transform* descendent);
    static R_Scene* getScene(R_Transform* xform);
    static void removeChild(R_Transform* parent, R_Transform* child);
    static void addChild(R_Transform* parent, R_Transform* child);
    static u32 numChildren(R_Transform* xform);
    static R_Transform* getChild(R_Transform* xform, u32 index);

    // Transform modification ------------------------------------------------
    static void rotateOnLocalAxis(R_Transform* xform, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(R_Transform* xform, glm::vec3 axis, f32 deg);

    // util -------------------------------------------------------------------
    static void print(R_Transform* xform, u32 depth);
    static void print(R_Transform* xform);
};

#define R_GEOMETRY_MAX_VERTEX_ATTRIBUTES 8
struct R_Geometry : public R_Component {
    GPU_Buffer gpu_vertex_buffers[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES]; // non-interleaved
    GPU_Buffer gpu_index_buffer;
    u8 vertex_attribute_num_components[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    // storage buffers for vertex pulling
    GPU_Buffer pull_buffers[SG_GEOMETRY_MAX_VERTEX_PULL_BUFFERS];
    WGPUBindGroup pull_bind_group;
    int vertex_count = -1; // if set, overrides vertex count from vertices
    bool pull_bind_group_dirty;

    static void init(R_Geometry* geo);

    static u32 indexCount(R_Geometry* geo);
    static u32 vertexCount(R_Geometry* geo);
    static u32 vertexAttributeCount(R_Geometry* geo);

    static void buildFromVertices(GraphicsContext* gctx, R_Geometry* geo,
                                  Vertices* vertices);

    static void setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo, u32 location,
                                   u32 num_components, f32* data, u32 data_count);

    // TODO if works, move into cpp
    // TODO move vertexPulling reflection check into state of ck ShaderDesc
    static bool usesVertexPulling(R_Geometry* geo);

    static void rebuildPullBindGroup(GraphicsContext* gctx, R_Geometry* geo,
                                     WGPUBindGroupLayout layout);

    static void setPulledVertexAttribute(GraphicsContext* gctx, R_Geometry* geo,
                                         u32 location, void* data, size_t size_bytes);

    static void setIndices(GraphicsContext* gctx, R_Geometry* geo, u32* indices,
                           u32 indices_count);
};

// =============================================================================
// R_Texture
// =============================================================================

struct R_Texture : public R_Component {
    Texture gpuTexture;
    SamplerConfig samplerConfig;

    static void init(R_Texture* texture);
};

void Material_batchUpdatePipelines(GraphicsContext* gctx, SG_ID main_scene);

// =============================================================================
// R_Shader
// =============================================================================

struct R_Shader : public R_Component {
    WGPUShaderModule vertex_shader_module;
    WGPUShaderModule fragment_shader_module;
    // e.g. [3, 3, 2, 4] for [POSITION, NORMAL, TEXCOORD_0, TANGENT]
    int vertex_layout[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    static void init(GraphicsContext* gctx, R_Shader* shader, const char* vertex_string,
                     const char* vertex_filepath, const char* fragment_string,
                     const char* fragment_filepath, int* vertex_layout);
    static void free(R_Shader* shader);
};

// =============================================================================
// R_Material
// =============================================================================

enum R_BindType : u32 {
    R_BIND_EMPTY = 0, // empty binding
    R_BIND_UNIFORM,
    R_BIND_SAMPLER,
    R_BIND_TEXTURE_ID,   // for scenegraph textures
    R_BIND_TEXTURE_VIEW, // default textures (e.g. white pixel)
    R_BIND_STORAGE
};

// TODO can we move R_Binding into .cpp
struct R_Binding {
    R_BindType type;
    size_t size; // size of data in bytes for UNIFORM and STORAGE types
    union {
        SG_ID textureID;
        WGPUTextureView textureView;
        SamplerConfig samplerConfig;
        GPU_Buffer storage_buffer;
    } as;
};

struct MaterialTextureView {
    // material texture view (not same as wgpu texture view)
    i32 texcoord; // 1 for TEXCOORD_1, etc.
    f32 strength; /* equivalent to strength for occlusion_texture */
    b32 hasTransform;
    // transform
    f32 offset[2];
    f32 rotation;
    f32 scale[2];

    static void init(MaterialTextureView* view);
};

struct R_Material : public R_Component {
    SG_MaterialPipelineState pso;

    b32 fresh_bind_group; // set if modified by chuck user, need to rebuild bind groups

    R_ID pipelineID; // renderpipeline this material belongs to
    bool pipeline_stale;

    // bindgroup state (uniforms, storage buffers, textures, samplers)
    R_Binding bindings[SG_MATERIAL_MAX_UNIFORMS];
    GPU_Buffer
      uniform_buffer; // maps 1:1 with uniform location, size =
                      // sizeof(SG_MaterialUniformData * SG_MATERIAL_MAX_UNIFORMS)
    WGPUBindGroup bind_group;

    // Arena bindings;         // array of R_Binding
    // Arena bindGroupEntries; // wgpu bindgroup entries. 1:1 with bindings
    // WGPUBindGroup bindGroup;

    // TODO: figure out how to include MaterialTextureView
    // maybe as part of the R_BIND_TEXTURE_ID binding,
    // because every texture needs a texture view

    // Arena uniformData;         // cpu-side uniform data buffer
    //                            // used by all bindings of type R_BIND_UNIFORM
    // WGPUBuffer gpuUniformBuff; // gpu-side uniform buffer
    // WGPUBufferDescriptor uniformBuffDesc;

    // textures (TODO: convert to SG_ID of SG_Texture)
    // SG_Texture* baseColorTexture;
    // MaterialTextureView baseColorTextureView;

    // SG_Texture* metallicRoughnessTexture;
    // MaterialTextureView metallicRoughnessTextureView;

    // shared textures (by all material types)
    // SG_Texture* normalTexture;
    // MaterialTextureView normalTextureView;
    // SG_Texture* occlusionTexture;
    // SG_Texture* emissiveTexture;

    // constructors ----------------------------------------------
    // static void init(GraphicsContext* gctx, R_Material* mat, R_MaterialConfig*
    // config);

    static void updatePSO(GraphicsContext* gctx, R_Material* mat,
                          SG_MaterialPipelineState* pso);

    // bind group fns --------------------------------------------
    static void rebuildBindGroup(R_Material* mat, GraphicsContext* gctx,
                                 WGPUBindGroupLayout layout);
    static void setBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                           R_BindType type, void* data, size_t bytes);
    static void removeBinding(R_Material* mat, u32 location)
    {
        ASSERT(false);
        // TODO
    }

    // TODO: reimplement
    static void setTextureAndSamplerBinding(R_Material* mat, u32 location,
                                            SG_ID textureID,
                                            WGPUTextureView defaultView);
};

// =============================================================================
// R_Scene
// =============================================================================

struct MaterialToGeometry {
    SG_ID material_id; // key
    Arena geo_ids;     // value, array of SG_IDs

    static int compare(const void* a, const void* b, void* udata)
    {
        return ((MaterialToGeometry*)a)->material_id
               - ((MaterialToGeometry*)b)->material_id;
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        MaterialToGeometry* key = (MaterialToGeometry*)item;
        return hashmap_xxhash3(&key->material_id, sizeof(key->material_id), seed0,
                               seed1);
    }

    static void free(void* item)
    {
        MaterialToGeometry* key = (MaterialToGeometry*)item;
        Arena::free(&key->geo_ids);
    }
};

struct GeometryToXformKey {
    SG_ID geo_id;
    SG_ID mat_id;
};

struct GeometryToXforms {
    GeometryToXformKey key;
    Arena xform_ids; // value, array of SG_IDs
    WGPUBindGroup xform_bind_group;
    GPU_Buffer xform_storage_buffer;
    bool stale;

    static int compare(const void* a, const void* b, void* udata)
    {
        GeometryToXforms* ga = (GeometryToXforms*)a;
        GeometryToXforms* gb = (GeometryToXforms*)b;
        return memcmp(&ga->key, &gb->key, sizeof(ga->key));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        GeometryToXforms* g2x = (GeometryToXforms*)item;
        return hashmap_xxhash3(&g2x->key, sizeof(g2x->key), seed0, seed1);
    }

    static void free(void* item)
    {
        GeometryToXforms* g2x = (GeometryToXforms*)item;
        Arena::free(&g2x->xform_ids);
        WGPU_RELEASE_RESOURCE(BindGroup, g2x->xform_bind_group);
        GPU_Buffer::destroy(&g2x->xform_storage_buffer);
    }

    static void rebuildBindGroup(GraphicsContext* gctx, R_Scene* scene,
                                 GeometryToXforms* g2x, WGPUBindGroupLayout layout,
                                 Arena* frame_arena);
};

struct R_Scene : R_Transform {
    glm::vec4 bg_color;

    hashmap* pipeline_to_material; // R_ID -> Arena of R_Material ids
    hashmap* material_to_geo;      // SG_ID -> Arena of geo ids
    hashmap* geo_to_xform;         // SG_ID -> Arena of xform ids (for each material)

    static void initFromSG(R_Scene* r_scene, SG_Command_SceneCreate* cmd);

    static void removeSubgraphFromRenderState(R_Scene* scene, R_Transform* xform);
    static void addSubgraphToRenderState(R_Scene* scene, R_Transform* xform);

    static GeometryToXforms* getPrimitive(R_Scene* scene, SG_ID geo_id, SG_ID mat_id)
    {
        GeometryToXformKey key = {};
        key.geo_id             = geo_id;
        key.mat_id             = mat_id;
        return (GeometryToXforms*)hashmap_get(scene->geo_to_xform, &key);
    }

    // static void free(R_Scene* scene);
};

// =============================================================================
// R_RenderPipeline
// =============================================================================

struct R_RenderPipeline /* NOT backed by SG_Component */ {
    R_ID rid;
    // RenderPipeline pipeline;
    WGPURenderPipeline gpu_pipeline;
    SG_MaterialPipelineState pso;
    // ptrdiff_t offset; // acts as an ID, offset in bytes into pipeline Arena

    Arena materialIDs; // array of SG_IDs

    WGPUBindGroup frame_group;
    static GPU_Buffer frame_uniform_buffer;

    /*
    possible optimizations:
    - keep material IDs with nonzero #primitive contiguous
      so we don't waste time iterating over empty materials
    */

    // static void init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
    //                  const SG_MaterialPipelineState* config, ptrdiff_t offset);
    static void init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
                     const SG_MaterialPipelineState* config);

    static void addMaterial(R_RenderPipeline* pipeline, R_Material* material);

    /// @brief Iterator for materials tied to render pipeline
    static size_t numMaterials(R_RenderPipeline* pipeline);
    static bool materialIter(R_RenderPipeline* pipeline, size_t* indexPtr,
                             R_Material** material);
};

// =============================================================================
// Component Manager API
// =============================================================================

R_Transform* Component_CreateTransform();
R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd);

R_Transform* Component_CreateMesh(SG_Command_Mesh_Create* cmd);

R_Scene* Component_CreateScene(SG_Command_SceneCreate* cmd);

R_Geometry* Component_CreateGeometry();
R_Geometry* Component_CreateGeometry(GraphicsContext* gctx, SG_Command_GeoCreate* cmd);

R_Shader* Component_CreateShader(GraphicsContext* gctx, SG_Command_ShaderCreate* cmd);

// R_Material* Component_CreateMaterial(GraphicsContext* gctx, R_MaterialConfig*
// config);
R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd);

R_Texture* Component_CreateTexture();

R_Component* Component_GetComponent(SG_ID id);
R_Transform* Component_GetXform(SG_ID id);
R_Scene* Component_GetScene(SG_ID id);
R_Geometry* Component_GetGeometry(SG_ID id);
R_Shader* Component_GetShader(SG_ID id);
R_Material* Component_GetMaterial(SG_ID id);
R_Texture* Component_GetTexture(SG_ID id);

// lazily created on-demand because of many possible shader variations
R_RenderPipeline* Component_GetPipeline(GraphicsContext* gctx,
                                        SG_MaterialPipelineState* pso);
R_RenderPipeline* Component_GetPipeline(R_ID rid);

// component iterators
// bool hashmap_scan(struct hashmap *map, bool (*iter)(const void *item, void
// *udata), void *udata);

// be careful to not delete components while iterating
// returns false upon reachign end of material arena
bool Component_MaterialIter(size_t* i, R_Material** material);
bool Component_RenderPipelineIter(size_t* i, R_RenderPipeline** renderPipeline);
int Component_RenderPipelineCount();

// component manager initialization
void Component_Init(GraphicsContext* gctx);
void Component_Free();

// TODO: add destroy functions. Remember to change offsets after swapping!
// should these live in the components?
// TODO: on xform destroy, set material/geo primitive to stale
// void Component_DestroyXform(u64 id);
/*
Enforcing pointer safety:
- hide all component initialization fns as static within component.cpp
    - only the manager can create/delete components
    - similar to how all memory allocations are routed through realloc
- all component accesses happen via IDs routed through the manager
    - IDs, unlike pointers, are safe to store
    - if the component created by that ID is deleted, the ID lookup will yield
    NULL, and the calling code will likely crash with a NULL pointer dereference
    (easy to debug)
- all deletions / GC are deferred to the VERY END of the frame
    - prevents bug where a component is deleted WHILE it is being used after an
    ID lookup
    - enforce hygiene of never storing / carrying pointers across frame
    boundaries (within is ok)
    - also enables a more controllable GC system
*/

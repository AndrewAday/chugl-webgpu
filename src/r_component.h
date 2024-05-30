#pragma once

#include "graphics.h"
#include "sg_command.h"
#include "sg_component.h"

#include "core/macros.h"
#include "core/memory.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

// TODO: write readme on architecture

// =============================================================================
// scenegraph data structures
// =============================================================================

struct Vertices;
struct R_Material;

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

struct R_Transform : public R_Component {
    // staleness flag has priority hiearchy, don't set directly!
    // instead use setStale()
    R_Transform_Staleness _stale;

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
    SG_ID _geoID;
    SG_ID _matID;

    static void init(R_Transform* transform);
    static void initFromSG(R_Transform* r_xform, SG_Command_CreateXform* cmd);

    static void setStale(R_Transform* xform, R_Transform_Staleness stale);

    static glm::mat4 localMatrix(R_Transform* xform);

    /// @brief decompose matrix into transform data
    static void setXformFromMatrix(R_Transform* xform, const glm::mat4& M);

    static void setXform(R_Transform* xform, const glm::vec3& pos,
                         const glm::quat& rot, const glm::vec3& sca);
    static void pos(R_Transform* xform, const glm::vec3& pos);
    static void rot(R_Transform* xform, const glm::quat& rot);
    static void sca(R_Transform* xform, const glm::vec3& sca);

    // updates all local/world matrices in the scenegraph
    static void rebuildMatrices(R_Transform* root, Arena* arena);

    // Scenegraph relationships ----------------------------------------------
    // returns if ancestor is somewhere in the parent chain of descendent,
    // including descendent itself
    static bool isAncestor(R_Transform* ancestor, R_Transform* descendent);
    static void removeChild(R_Transform* parent, R_Transform* child);
    static void addChild(R_Transform* parent, R_Transform* child);
    static u32 numChildren(R_Transform* xform);
    static R_Transform* getChild(R_Transform* xform, u32 index);

    // Transform modification ------------------------------------------------
    static void rotateOnLocalAxis(R_Transform* xform, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(R_Transform* xform, glm::vec3 axis, f32 deg);

    // util -------------------------------------------------------------------
    static void print(R_Transform* xform, u32 depth = 0);
};

struct R_Geometry : public R_Component {

    // IndexBuffer indexBuffer;
    // VertexBuffer vertexBuffer;

    WGPUBuffer gpuIndexBuffer;
    WGPUBuffer gpuVertexBuffer; // non-interleaved, contiguous
    WGPUBufferDescriptor indexBufferDesc;
    WGPUBufferDescriptor vertexBufferDesc;

    u32 numVertices;
    u32 numIndices;

    // // associated xform instances
    // Arena xformIDs;

    // // bindgroup
    // WGPUBindGroupEntry bindGroupEntry;
    // WGPUBindGroup bindGroup;
    // WGPUBuffer storageBuffer;
    // u32 storageBufferCap; // capacity in number of FrameUniforms NOT bytes
    // bool staleBindGroup;  // true if storage buffer needs to be rebuilt with
    // new
    //                       // world matrices

    static void init(R_Geometry* geo);
    static void buildFromVertices(GraphicsContext* gctx, R_Geometry* geo,
                                  Vertices* vertices);
    // static u64 numInstances(R_Geometry* geo);

    // uploads xform data to storage buffer
};

// =============================================================================
// R_Texture
// =============================================================================

struct R_Texture : public R_Component {
    Texture gpuTexture;
    SamplerConfig samplerConfig;

    static void init(R_Texture* texture);
};

// =============================================================================
// Material_Primitive
// =============================================================================

// groups xform data and geometry under a given R_Material instance
struct Material_Primitive {
    // geometry
    SG_ID geoID;
    // material this belongs to
    SG_ID matID;

    // associated xform instances
    Arena xformIDs; // array of SG_ID

    // bindgroup
    WGPUBindGroupEntry bindGroupEntry;
    WGPUBindGroup bindGroup;
    WGPUBuffer storageBuffer;
    u32 storageBufferCap; // capacity in number of FrameUniforms NOT bytes
    b32 stale;            // true if storage buffer needs to be rebuilt with new
                          // world matrices

    static void init(Material_Primitive* prim, R_Material* mat, SG_ID geoID);
    static void free(Material_Primitive* prim);
    static u32 numInstances(Material_Primitive* prim);
    static void addXform(Material_Primitive* prim, R_Transform* xform);
    static void removeXform(Material_Primitive* prim, R_Transform* xform);

    // recreates storagebuffer based on #xformIDs and rebuilds bindgroup
    // populates storage buffer with new xform data
    // only creates new storage buffer if #xformIDs grows or an existing xformID
    // is moved
    static void rebuildBindGroup(GraphicsContext* gctx,
                                 Material_Primitive* prim,
                                 WGPUBindGroupLayout layout, Arena* arena);
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
        ptrdiff_t uniformOffset; // offset into uniform buffer
        void* storageData;       // cpu-side storage buffer data
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

// base material properties that determine pipeline selection
struct R_MaterialConfig {
    SG_MaterialType material_type = SG_MATERIAL_PBR;
    SG_ID shaderID                = 0; // 0 for built-in shaders.
    b32 doubleSided               = false;
};

// TODO: somehow make this interop with render pipeline and shader
// material instance component
struct R_Material : public R_Component {
    R_MaterialConfig config;
    b32 stale; // set if modified by chuck user, need to rebuild bind groups

    R_ID pipelineID; // renderpipeline this material belongs to

    Arena bindings;         // array of R_Binding
    Arena bindGroupEntries; // wgpu bindgroup entries. 1:1 with bindings
    WGPUBindGroup bindGroup;

    // TODO: figure out how to include MaterialTextureView
    // maybe as part of the R_BIND_TEXTURE_ID binding,
    // because every texture needs a texture view

    Arena uniformData;         // cpu-side uniform data buffer
                               // used by all bindings of type R_BIND_UNIFORM
    WGPUBuffer gpuUniformBuff; // gpu-side uniform buffer
    WGPUBufferDescriptor uniformBuffDesc;

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

    // glm::vec4 baseColor;
    // f32 metallicFactor;
    // f32 roughnessFactor;

    // primitive data ---------------
    Arena primitives; // array of Material_Primitive (geo + xform)

    // constructors ----------------------------------------------
    static void init(GraphicsContext* gctx, R_Material* mat,
                     R_MaterialConfig* config);

    // bind group fns --------------------------------------------
    static void rebuildBindGroup(R_Material* mat, GraphicsContext* gctx,
                                 WGPUBindGroupLayout layout);
    static void setBinding(R_Material* mat, u32 location, R_BindType type,
                           void* data, size_t bytes);

    static void setTextureAndSamplerBinding(R_Material* mat, u32 location,
                                            SG_ID textureID,
                                            WGPUTextureView defaultView);

    // functions for adding/removing primitives ------------------
    static u32 numPrimitives(R_Material* mat);
    static void addPrimitive(R_Material* mat, R_Geometry* geo,
                             R_Transform* xform);
    static void removePrimitive(R_Material* mat, R_Geometry* geo,
                                R_Transform* xform);
    static void markPrimitiveStale(R_Material* mat, R_Transform* xform);
    static bool primitiveIter(R_Material* mat, size_t* indexPtr,
                              Material_Primitive** primitive);
};

// =============================================================================
// R_Scene
// =============================================================================
struct R_Scene : R_Transform {
    glm::vec4 bg_color;

    static void initFromSG(R_Scene* r_scene, SG_Command_SceneCreate* cmd);
    // static void free(R_Scene* scene);
};

// =============================================================================
// R_RenderPipeline
// =============================================================================

struct R_RenderPipeline /* NOT backed by SG_Component */ {
    R_ID rid;
    RenderPipeline pipeline;
    R_MaterialConfig config;
    ptrdiff_t offset; // acts as an ID, offset in bytes into pipeline Arena

    Arena materialIDs; // array of SG_IDs

    /*
    possible optimizations:
    - keep material IDs with nonzero #primitive contiguous
      so we don't waste time iterating over empty materials
    */

    static void init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
                     const R_MaterialConfig* config, ptrdiff_t offset);

    static void addMaterial(R_RenderPipeline* pipeline, R_Material* material);

    /// @brief Iterator for materials tied to render pipeline
    static bool materialIter(R_RenderPipeline* pipeline, size_t* indexPtr,
                             R_Material** material);
};

// =============================================================================
// Component Manager API
// =============================================================================

R_Transform* Component_CreateTransform();
R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd);
R_Scene* Component_CreateScene(SG_Command_SceneCreate* cmd);

R_Geometry* Component_CreateGeometry();
R_Geometry* Component_CreateGeometry(GraphicsContext* gctx,
                                     SG_Command_GeoCreate* cmd);

R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     R_MaterialConfig* config);
R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd);

R_Texture* Component_CreateTexture();

R_Component* Component_GetComponent(SG_ID id);
R_Transform* Component_GetXform(SG_ID id);
R_Scene* Component_GetScene(SG_ID id);
R_Geometry* Component_GetGeo(SG_ID id);
R_Material* Component_GetMaterial(SG_ID id);
R_Texture* Component_GetTexture(SG_ID id);

// lazily created on-demand because of many possible shader variations
R_RenderPipeline* Component_GetPipeline(GraphicsContext* gctx,
                                        R_MaterialConfig* config);
R_RenderPipeline* Component_GetPipeline(R_ID rid);

// component iterators
// bool hashmap_scan(struct hashmap *map, bool (*iter)(const void *item, void
// *udata), void *udata);

// be careful to not delete components while iterating
// returns false upon reachign end of material arena
bool Component_MaterialIter(size_t* i, R_Material** material);
bool Component_RenderPipelineIter(size_t* i, R_RenderPipeline** renderPipeline);

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

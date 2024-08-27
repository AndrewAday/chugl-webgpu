#pragma once

#include "graphics.h"
#include "sg_command.h"
#include "sg_component.h"

#include "core/macros.h"
#include "core/memory.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <unordered_map>

// freetype font library
#include <ft2build.h>
#include FT_FREETYPE_H

// =============================================================================
// scenegraph data structures
// =============================================================================

struct Vertices;
struct R_Material;
struct R_Scene;
struct R_Font;
struct hashmap;

typedef SG_ID R_ID; // negative for R_Components NOT mapped to SG_Components

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
    static void setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo, u32 location,
                                   u32 num_components_per_attrib, void* data,
                                   size_t size);

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
    Texture gpu_texture;
    // u32 width;
    // u32 height;
    // u32 depth;
    // u32 mip_level_count;
    // WGPUTexture texture;
    // WGPUTextureView view;
    // SamplerConfig samplerConfig; // TODO texture: maybe have sampler separate?
    u64 generation = 0; // incremented every time texture is modified

    SG_TextureDesc desc;

    static void init(R_Texture* texture); // called by Renderer-Tester only

    static void write(GraphicsContext* gctx, R_Texture* texture, void* data, int width,
                      int height)
    {
        Texture::initFromPixelData(gctx, &texture->gpu_texture, texture->name.c_str(),
                                   data, width, height, true,
                                   texture->desc.usage_flags);
        texture->generation++;
    }

    static void fromFile(GraphicsContext* gctx, R_Texture* texture,
                         const char* filepath);
};

void Material_batchUpdatePipelines(GraphicsContext* gctx, FT_Library ft_lib,
                                   R_Font* default_font, SG_ID main_scene_id);

// =============================================================================
// R_Shader
// =============================================================================

struct R_Shader : public R_Component {
    WGPUShaderModule vertex_shader_module;
    WGPUShaderModule fragment_shader_module;
    // e.g. [3, 3, 2, 4] for [POSITION, NORMAL, TEXCOORD_0, TANGENT]
    WGPUVertexFormat vertex_layout[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    static void init(GraphicsContext* gctx, R_Shader* shader, const char* vertex_string,
                     const char* vertex_filepath, const char* fragment_string,
                     const char* fragment_filepath, WGPUVertexFormat* vertex_layout,
                     int vertex_layout_count);
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
    R_BIND_STORAGE,
    R_BIND_STORAGE_EXTERNAL // pointer to external storage buffer (ref)
};

// TODO can we move R_Binding into .cpp
struct R_Binding {
    R_BindType type;
    size_t size;    // size of data in bytes for UNIFORM and STORAGE types
    u64 generation; // currently only used for textures, track generation so we know
                    // when to rebuild BindGroup
                    // eventually can use to track GPU_Buffer generation
    union {
        SG_ID textureID;
        WGPUTextureView textureView;
        SamplerConfig samplerConfig;
        GPU_Buffer storage_buffer;
        GPU_Buffer* storage_external; // ptr here might be dangerous...
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
    GPU_Buffer uniform_buffer; // maps 1:1 with uniform location, initializesd in
                               // Component_MaterialCreate
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
    static void setUniformBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  void* data, size_t bytes)
    {
        setBinding(gctx, mat, location, R_BIND_UNIFORM, data, bytes);
    }
    static void setSamplerBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  SG_Sampler sampler);
    static void setTextureBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  SG_ID texture_id);
    static void setExternalStorageBinding(GraphicsContext* gctx, R_Material* mat,
                                          u32 location, GPU_Buffer* buffer);
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
// R_Camera
// =============================================================================

struct R_Camera : public R_Transform {
    SG_CameraParams params;
    GPU_Buffer frame_uniform_buffer;

    static glm::mat4 projectionMatrix(R_Camera* camera, f32 aspect)
    {
        switch (camera->params.camera_type) {
            case SG_CameraType_PERPSECTIVE:
                return glm::perspective(camera->params.fov_radians, aspect,
                                        camera->params.near_plane,
                                        camera->params.far_plane);
            case SG_CameraType_ORTHOGRAPHIC: {
                float width  = camera->params.size * aspect;
                float height = camera->params.size;
                return glm::ortho( // extents in WORLD SPACE units
                  -width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f,
                  camera->params.near_plane, camera->params.far_plane);
            }
            default: ASSERT(false); return glm::mat4(1.0f);
        }
    }

    static glm::mat4 viewMatrix(R_Camera* cam)
    {
        ASSERT(cam->_stale == R_Transform_STALE_NONE);
        return glm::inverse(cam->world);
        // accounts for scale
        // return glm::inverse(modelMatrix(entity));

        // optimized version for camera only (doesn't take scale into account)
        // glm::mat4 invT = glm::translate(MAT_IDENTITY, -cam->_pos);
        // glm::mat4 invR = glm::toMat4(glm::conjugate(cam->_rot));
        // return invR * invT;
    }
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

    SG_ID main_camera_id;

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

    // RenderPass refactor: removing
    // bindgroup now in App->frame_uniforms_map
    // frame uniform buffer now stored per R_Camera
    // keeping these around to handle case of null camera
    // TODO remove after removing null default camera in chugl (and implementing camera
    // controllers)
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
                     const SG_MaterialPipelineState* config, int msaa_sample_count = 4);

    static void addMaterial(R_RenderPipeline* pipeline, R_Material* material);

    /// @brief Iterator for materials tied to render pipeline
    static size_t numMaterials(R_RenderPipeline* pipeline);
    static bool materialIter(R_RenderPipeline* pipeline, size_t* indexPtr,
                             R_Material** material);
};

// =============================================================================
// R_Pass
// =============================================================================

#if 0 // notes from hdr.c

// pipeline:
// offscreen_pass --> bloom filter pass --> render pass (onscreen pass)

enum wgpu_render_pass_attachment_type_t {
  WGPU_RENDER_PASS_COLOR_ATTACHMENT_TYPE         = 0x00000001,
  WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_TYPE = 0x00000002,
};

struct frame_buffer_attachment_t {
  WGPUTexture texture;
  WGPUTextureView texture_view;
  WGPUTextureFormat format;
};

struct { // can generalize this to all passes, not just offscreen
  uint32_t width, height;
  frame_buffer_attachment_t color[2]; // why 2?
  frame_buffer_attachment_t depth;

  struct { // everything needed by WGPURenderPassDescriptor
    WGPURenderPassColorAttachment color_attachment[2];
    WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
    WGPURenderPassDescriptor render_pass_descriptor;
  } render_pass_desc;

  WGPUSampler sampler; // what is sampler for?
} offscreen_pass = {0};

struct { // screen pass? hence no need for depth buffer
  uint32_t width, height;
  frame_buffer_attachment_t color[1];
  struct {
    WGPURenderPassColorAttachment color_attachment[1];
    WGPURenderPassDescriptor render_pass_descriptor;
  } render_pass_desc;
  WGPUSampler sampler;
} filter_pass;

// sampler params (nearest, not linear! because we are sampling 1:1)
    filter_pass.sampler = wgpuDeviceCreateSampler(
      device, &(WGPUSamplerDescriptor){
                              .label         = "Filter pass texture sampler",
                              .addressModeU  = WGPUAddressMode_ClampToEdge,
                              .addressModeV  = WGPUAddressMode_ClampToEdge,
                              .addressModeW  = WGPUAddressMode_ClampToEdge,
                              .minFilter     = WGPUFilterMode_Nearest,
                              .magFilter     = WGPUFilterMode_Nearest,
                              .mipmapFilter  = WGPUMipmapFilterMode_Linear,
                              .lodMinClamp   = 0.0f,
                              .lodMaxClamp   = 1.0f,
                              .maxAnisotropy = 1,
                            });

// populates framebufferAttachment params
// improvement: just pass usage_flags directly instead of attachment_type
void create_attachment(wgpu_context_t* wgpu_context, const char* texture_label,
                       WGPUTextureFormat format,
                       wgpu_render_pass_attachment_type_t attachment_type,
                       frame_buffer_attachment_t* attachment)
{
  // Create the texture extent
  WGPUExtent3D texture_extent = { // TODO this takes window width/height params
    .width              = width 
    .height             = height
    .depthOrArrayLayers = 1,
  };

  // Texture usage flags
  WGPUTextureUsageFlags usage_flags = WGPUTextureUsage_RenderAttachment;
  if (attachment_type == WGPU_RENDER_PASS_COLOR_ATTACHMENT_TYPE) {
    usage_flags = usage_flags | WGPUTextureUsage_TextureBinding; // allow texture to be bound and sampled in shader
  }
  else if (attachment_type == WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_TYPE) {
    usage_flags = usage_flags | WGPUTextureUsage_CopySrc; // why copy src?? where's the copy
  }

  // Texture format
  attachment->format = format;

  // Create the texture
  WGPUTextureDescriptor texture_desc = {
    .label         = texture_label,
    .size          = texture_extent,
    .mipLevelCount = 1,
    .sampleCount   = 1, // TODO add sample count
    .dimension     = WGPUTextureDimension_2D,
    .format        = attachment->format,
    .usage         = usage_flags,
  };
  attachment->texture
    = wgpuDeviceCreateTexture(wgpu_context->device, &texture_desc);
  ASSERT(attachment->texture);

  // Create the texture view
  WGPUTextureViewDescriptor texture_view_dec = {
    .label           = "Texture view",
    .dimension       = WGPUTextureViewDimension_2D,
    .format          = texture_desc.format,
    .baseMipLevel    = 0,
    .mipLevelCount   = 1,
    .baseArrayLayer  = 0,
    .arrayLayerCount = 1,
    .aspect          = WGPUTextureAspect_All,
  };
  attachment->texture_view
    = wgpuTextureCreateView(attachment->texture, &texture_view_dec);
  ASSERT(attachment->texture_view);
}

#endif

struct Framebuffer {
    u32 width        = 0;
    u32 height       = 0;
    int sample_count = 0;

    WGPUTexture depth_tex;
    WGPUTextureView depth_view;
    WGPUTexture color_tex;
    WGPUTextureView color_view;

    // nvm, resolve target comes from chugl texture
    // WGPUTexture resolve_tex; // sample_count == 1
    // WGPUTextureView resolve_view;

    static void createAttachment(GraphicsContext* gctx, WGPUTextureFormat format,
                                 WGPUTextureUsageFlags usage_flags, u32 width,
                                 u32 height, int sample_count, WGPUTexture* out_tex,
                                 WGPUTextureView* out_view)
    {
        WGPUExtent3D texture_extent = { width, height, 1 };

        // Texture usage flags
        usage_flags |= WGPUTextureUsage_RenderAttachment;

        // Create the texture
        WGPUTextureDescriptor texture_desc = {};
        texture_desc.label                 = NULL;
        texture_desc.size                  = texture_extent;
        texture_desc.mipLevelCount         = 1;
        texture_desc.sampleCount           = sample_count;
        texture_desc.dimension             = WGPUTextureDimension_2D;
        texture_desc.format                = format;
        texture_desc.usage                 = usage_flags;

        WGPU_RELEASE_RESOURCE(Texture, *out_tex);
        *out_tex = wgpuDeviceCreateTexture(gctx->device, &texture_desc);
        ASSERT(*out_tex);

        // Create the texture view
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.label                     = NULL;
        texture_view_desc.dimension                 = WGPUTextureViewDimension_2D;
        texture_view_desc.format                    = texture_desc.format;
        texture_view_desc.baseMipLevel              = 0;
        texture_view_desc.mipLevelCount             = 1;
        texture_view_desc.baseArrayLayer            = 0;
        texture_view_desc.arrayLayerCount           = 1;
        texture_view_desc.aspect                    = WGPUTextureAspect_All;

        WGPU_RELEASE_RESOURCE(TextureView, *out_view);
        *out_view = wgpuTextureCreateView(*out_tex, &texture_view_desc);
        ASSERT(*out_view);
    }

    // rebuilds framebuffer attachment textures
    static void rebuild(GraphicsContext* gctx, Framebuffer* fb, u32 width, u32 height,
                        int sample_count, WGPUTextureFormat color_format)
    {
        bool texture_resized      = (fb->width != width || fb->height != height);
        bool sample_count_changed = fb->sample_count != sample_count;
        // todo support change in resolve target SG_ID
        if (texture_resized || sample_count_changed) {
            log_debug("rebuilding framebuffer");
            fb->width        = width;
            fb->height       = height;
            fb->sample_count = sample_count;

            // recreate color target TODO get format and usage from sgid
            // for now locking down to hdr
            Framebuffer::createAttachment(
              gctx, color_format, WGPUTextureUsage_TextureBinding, width, height,
              sample_count, &fb->color_tex, &fb->color_view);

            // recreate depth target
            Framebuffer::createAttachment(
              gctx, WGPUTextureFormat_Depth24PlusStencil8, WGPUTextureUsage_None, width,
              height, sample_count, &fb->depth_tex, &fb->depth_view);

            // TODO create resolve
        }
    }
};

struct R_Pass : public R_Component {
    SG_Pass sg_pass;

    // RenderPass params
    WGPURenderPassColorAttachment color_attachments[1];
    WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
    WGPURenderPassDescriptor render_pass_desc;
    // RenderPass framebuffer
    Framebuffer framebuffer;

    // todo gpass: write without support for user-provided textures
    // refactor after re-writing texture class

    // if window size has changed, lazily reconstruct depth/stencil and color targets
    // update DepthStencilAttachment and ColorAttachment params based on ChuGL
    // RenderPass params
    // this should be called right before _R_RenderScene() for the given pass.
    // R_Pass.sg_pass is assumed to be updated (memcopied in the updatePass Command),
    // but not the gpu-specific parameters of R_Pass
    static void updateRenderPassDesc(GraphicsContext* gctx, R_Pass* pass,
                                     u32 window_width, u32 window_height,
                                     int sample_count, WGPUTextureView swapchain_view,
                                     WGPUTextureFormat view_format)
    {
        ASSERT(pass->sg_pass.pass_type == SG_PassType_Render);

        // handle resize
        Framebuffer::rebuild(gctx, &pass->framebuffer, window_width, window_height,
                             sample_count, view_format);

        // for now, we always set renderpass depth/stencil and color descriptors (even
        // if they haven't changed) to simplify state management
        { // depth
            pass->depth_stencil_attachment.view = pass->framebuffer.depth_view;
            // defaults for render pass depth/stencil attachment
            // The initial value of the depth buffer, meaning "far"
            pass->depth_stencil_attachment.depthClearValue = 1.0f;
            pass->depth_stencil_attachment.depthLoadOp     = WGPULoadOp_Clear;
            pass->depth_stencil_attachment.depthStoreOp    = WGPUStoreOp_Store;
            // we could turn off writing to the depth buffer globally here
            pass->depth_stencil_attachment.depthReadOnly = false;

            // Stencil setup, mandatory but unused
            pass->depth_stencil_attachment.stencilClearValue = 0;
            pass->depth_stencil_attachment.stencilLoadOp     = WGPULoadOp_Clear;
            pass->depth_stencil_attachment.stencilStoreOp    = WGPUStoreOp_Store;
            pass->depth_stencil_attachment.stencilReadOnly   = false;
        }

        { // color
            // defaults for render pass color attachment
            WGPURenderPassColorAttachment* ca = &pass->color_attachments[0];
            *ca                               = {};

            // view and resolve set in GraphicsContext::prepareFrame()
            ca->view
              = pass->framebuffer.color_view; // multisampled view, for now lock down
            ca->resolveTarget = swapchain_view;

#ifdef __EMSCRIPTEN__
            ca->depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
            // TODO chugl API to set loadOp and clearcolor
            ca->loadOp     = WGPULoadOp_Clear; // WGPULoadOp_Load does not clear
            ca->storeOp    = WGPUStoreOp_Store;
            ca->clearValue = WGPUColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        }

        { // renderpass desc
            pass->render_pass_desc                      = {};
            pass->render_pass_desc.label                = pass->sg_pass.name;
            pass->render_pass_desc.colorAttachmentCount = 1;
            pass->render_pass_desc.colorAttachments     = pass->color_attachments;
            pass->render_pass_desc.depthStencilAttachment
              = &pass->depth_stencil_attachment;
        }
    }
};

// =============================================================================
// R_Font
// =============================================================================

struct Glyph {
    FT_UInt index;
    i32 bufferIndex;

    i32 curveCount;

    // Important glyph metrics in font units.
    FT_Pos width, height;
    FT_Pos bearingX;
    FT_Pos bearingY;
    FT_Pos advance;
};

struct BufferGlyph {
    i32 start, count; // range of bezier curves belonging to this glyph
};
static_assert(sizeof(BufferGlyph) == (2 * sizeof(i32)), "bufferglyph size");

struct BufferCurve {
    float x0, y0, x1, y1, x2, y2;
};
static_assert(sizeof(BufferCurve) == 6 * sizeof(float), "buffercurve size");

struct BoundingBox {
    float minX, minY, maxX, maxY;
};

struct R_Text : public R_Transform {
    std::string text;
    std::string font_path;
    glm::vec2 control_points;
    float vertical_spacing;
};

struct R_Font {
    std::string font_path;
    FT_Face face; // TODO multiplex faces across R_Font. multiple R_Font with same font
                  // but different text can share the same face

    FT_Int32 loadFlags;
    FT_Kerning_Mode kerningMode;

    // Size of the em square used to convert metrics into em-relative values,
    // which can then be scaled to the worldSize. We do the scaling ourselves in
    // floating point to support arbitrary world sizes (whereas the fixed-point
    // numbers used by FreeType do not have enough resolution if the world size
    // is small).
    // Following the FreeType convention, if hinting (and therefore scaling) is enabled,
    // this value is in 1/64th of a pixel (compatible with 26.6 fixed point numbers).
    // If hinting/scaling is not enabled, this value is in font units.
    float emSize;

    float worldSize = 1.0f;

    GPU_Buffer glyph_buffer;
    GPU_Buffer curve_buffer;

    std::vector<BufferGlyph> bufferGlyphs;
    std::vector<BufferCurve> bufferCurves;
    std::unordered_map<u32, Glyph> glyphs;

    // The glyph quads are expanded by this amount to enable proper
    // anti-aliasing. Value is relative to emSize.
    float dilation = 0.1f;

    // given a text object, updates its geo vertex buffers
    // and material bindgroup
    static void updateText(GraphicsContext* gctx, R_Font* font, R_Text* text);
    static bool init(GraphicsContext* gctx, FT_Library library, R_Font* font,
                     const char* font_path);

    static void free(R_Font* text)
    {
        GPU_Buffer::destroy(&text->glyph_buffer);
        GPU_Buffer::destroy(&text->curve_buffer);
        FT_Done_Face(text->face);
    }

    static void prepareGlyphsForText(GraphicsContext* gctx, R_Font* font,
                                     const char* text);

    // given text and a starting model-space coordinate (x,y)
    // reconstructs the vertex and index buffers for the text
    // (used to batch draw a single GText object)
    static void rebuildVertexBuffers(R_Font* font, const char* mainText, float x,
                                     float y, Arena* positions, Arena* uvs,
                                     Arena* glyph_indices, Arena* indices,
                                     float verticalScale = 1.0f);

    BoundingBox measure(float x, float y, const char* text, float verticalScale = 1.0f);
};

// =============================================================================
// Component Manager API
// =============================================================================

R_Transform* Component_CreateTransform();
R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd);

R_Transform* Component_CreateMesh(SG_Command_Mesh_Create* cmd);
R_Camera* Component_CreateCamera(GraphicsContext* gctx, SG_Command_CameraCreate* cmd);
R_Text* Component_CreateText(GraphicsContext* gctx, FT_Library ft,
                             SG_Command_TextRebuild* cmd);

R_Scene* Component_CreateScene(SG_Command_SceneCreate* cmd);

R_Geometry* Component_CreateGeometry();
R_Geometry* Component_CreateGeometry(GraphicsContext* gctx, SG_Command_GeoCreate* cmd);

R_Shader* Component_CreateShader(GraphicsContext* gctx, SG_Command_ShaderCreate* cmd);

// R_Material* Component_CreateMaterial(GraphicsContext* gctx, R_MaterialConfig*
// config);
R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd);

R_Texture* Component_CreateTexture();
R_Texture* Component_CreateTexture(GraphicsContext* gctx,
                                   SG_Command_TextureCreate* cmd);
R_Pass* Component_CreatePass(SG_ID pass_id);

R_Component* Component_GetComponent(SG_ID id);
R_Transform* Component_GetXform(SG_ID id);
R_Scene* Component_GetScene(SG_ID id);
R_Geometry* Component_GetGeometry(SG_ID id);
R_Shader* Component_GetShader(SG_ID id);
R_Material* Component_GetMaterial(SG_ID id);
R_Texture* Component_GetTexture(SG_ID id);
R_Camera* Component_GetCamera(SG_ID id);
R_Text* Component_GetText(SG_ID id);
R_Font* Component_GetFont(GraphicsContext* gctx, FT_Library library,
                          const char* font_path);
R_Pass* Component_GetPass(SG_ID id);

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

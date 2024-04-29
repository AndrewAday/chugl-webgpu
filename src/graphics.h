#pragma once

#include "core/macros.h"

#include <glfw3webgpu/glfw3webgpu.h>
#include <webgpu/webgpu.h>

#define WGPU_RELEASE_RESOURCE(Type, Name)                                      \
    if (Name) {                                                                \
        wgpu##Type##Release(Name);                                             \
        Name = NULL;                                                           \
    }

#define WGPU_DESTROY_RESOURCE(Type, Name)                                      \
    ASSERT(Name != NULL);                                                      \
    wgpu##Type##Destroy(Name);

// ============================================================================
// Context
// =========================================================================================
// TODO request maximum device feature limits
struct GraphicsContext {
    void* base; //
    // WebGPU API objects --------
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;

    WGPUSwapChain swapChain;
    WGPUTextureFormat swapChainFormat;

    WGPUTexture depthTexture;
    WGPUTextureView depthTextureView;

    // Per frame resources --------
    WGPUTextureView backbufferView;
    WGPUCommandEncoder commandEncoder;
    WGPURenderPassColorAttachment colorAttachment;
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    WGPURenderPassDescriptor renderPassDesc;
    WGPURenderPassEncoder renderPassEncoder;
    WGPUCommandBuffer commandBuffer;

    // Window and surface --------
    WGPUSurface surface;

    // Methods --------
    static bool init(GraphicsContext* context, GLFWwindow* window);
    static WGPURenderPassEncoder prepareFrame(GraphicsContext* ctx);
    static void presentFrame(GraphicsContext* ctx);
    static void resize(GraphicsContext* ctx, u32 width, u32 height);
    static void release(GraphicsContext* ctx);
};

// ============================================================================
// Buffers
// =============================================================================

struct VertexBuffer {
    WGPUBuffer buf;
    WGPUBufferDescriptor desc;

    // hold copy of original data?

    static void init(GraphicsContext* ctx, VertexBuffer* buf, u64 data_length,
                     const f32* data, // force float data for now
                     const char* label);
};

struct IndexBuffer {
    WGPUBuffer buf;
    WGPUBufferDescriptor desc;

    static void init(GraphicsContext* ctx, IndexBuffer* buf, u64 data_length,
                     const u32* data, // force float data for now
                     const char* label);
};

// ============================================================================
// Attributes
// ============================================================================

#define VERTEX_BUFFER_LAYOUT_MAX_ENTRIES 8
// TODO request this in device limits
// force de-interleaved data. i.e. each attribute has its own buffer
struct VertexBufferLayout {
    WGPUVertexBufferLayout layouts[VERTEX_BUFFER_LAYOUT_MAX_ENTRIES];
    WGPUVertexAttribute attributes[VERTEX_BUFFER_LAYOUT_MAX_ENTRIES];
    u8 attribute_count;

    static void init(VertexBufferLayout* layout, u8 attribute_count,
                     u32* attribute_strides // stride in count NOT bytes
    );
};

// ============================================================================
// Shaders
// ============================================================================

struct ShaderModule {
    WGPUShaderModule module;
    WGPUShaderModuleDescriptor desc;

    // only support wgsl for now (can change into union later)
    WGPUShaderModuleWGSLDescriptor wgsl_desc;

    static void init(GraphicsContext* ctx, ShaderModule* module,
                     const char* code, const char* label);

    static void release(ShaderModule* module);

    static void compilationCallback(WGPUCompilationInfoRequestStatus status,
                                    const WGPUCompilationInfo* info,
                                    void* userdata);
};

// ============================================================================
// Bind Group
// ============================================================================

struct BindGroup {
    WGPUBindGroup bindGroup;
    WGPUBindGroupDescriptor desc;
    WGPUBuffer uniformBuffer;

    // unsupported:
    // WGPUSampler sampler;
    // WGPUTextureView textureView;

    static void init(GraphicsContext* ctx, BindGroup* bindGroup,
                     WGPUBindGroupLayout layout, u64 bufferSize);
};

// ============================================================================
// Render Pipeline
// ============================================================================

struct RenderPipeline {
    WGPURenderPipeline pipeline;
    WGPURenderPipelineDescriptor desc;

    // binding layouts: per frame, per material, per draw
    WGPUBindGroupLayout bindGroupLayouts[3];

    // possible optimization: only store material bind group in render pipeline
    // all pipelines can share global bind groups for per frame
    // and renderable models need their own per draw bind groups
    // each pipeline will need the frame/material/draw layouts
    // each pipeline has a unique per-material layout
    // the actual bind groups are stored elsewhere
    BindGroup bindGroups[1]; // just PER_FRAME_GROUP

    static void init(GraphicsContext* ctx, RenderPipeline* pipeline,
                     const char* vertexShaderCode,
                     const char* fragmentShaderCode);

    static void release(RenderPipeline* pipeline);
};

// ============================================================================
// Depth Texture
// ============================================================================

// TODO: unused, remove?

struct DepthTexture {
    WGPUTexture texture;
    WGPUTextureView view;
    WGPUTextureFormat format;

    static void init(GraphicsContext* ctx, DepthTexture* depthTexture,
                     WGPUTextureFormat format);

    static void release(DepthTexture* depthTexture);
};

// ============================================================================
// Texture
// ============================================================================

struct Texture {
    u32 width;
    u32 height;
    u32 depth;
    u32 mip_level_count;

    WGPUTextureFormat format;
    WGPUTextureDimension dimension;
    WGPUTexture texture;
    WGPUTextureView view;
    WGPUSampler sampler;

    static void initFromFile(GraphicsContext* ctx, Texture* texture,
                             const char* filename, bool genMipMaps);

    static void release(Texture* texture);
};

// ============================================================================
// Material
// ============================================================================

/// @brief Material instance provides uniforms/textures/etc
/// and bindGroup for a given render pipeline.
/// Each render pipeline is associated with particular material type
struct Material {
    WGPUBindGroup bindGroup;
    WGPUBuffer uniformBuffer;
    // glm::vec4 color;
    Texture* texture; // multiple materials can share same texture

    // bind group entries
    WGPUBindGroupEntry entries[3]; // uniforms, texture, sampler
    WGPUBindGroupDescriptor desc;

    // TODO: store MaterialUniforms struct (struct inheritance, like Obj)

    static void init(GraphicsContext* ctx, Material* material,
                     RenderPipeline* pipeline, Texture* texture);

    static void setTexture(GraphicsContext* ctx, Material* material,
                           Texture* texture);

    static void release(Material* material);
};
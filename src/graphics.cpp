#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <stb/stb_image.h>

#include "core/log.h"
#include "core/memory.h"

#include "graphics.h"
#include "shaders.h"

void printBackend()
{
#if defined(WEBGPU_BACKEND_DAWN)
    std::cout << "Backend: Dawn" << std::endl;
#elif defined(WEBGPU_BACKEND_WGPU)
    std::cout << "Backend: WGPU" << std::endl;
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    std::cout << "Backend: Emscripten" << std::endl;
#else
    std::cout << "Backend: Unknown" << std::endl;
#endif
}

void printAdapterInfo(WGPUAdapter adapter)
{
    WGPUAdapterProperties properties;
    wgpuAdapterGetProperties(adapter, &properties);

    std::cout << "Adapter name: " << properties.name << std::endl;
    std::cout << "Adapter vendor: " << properties.vendorName << std::endl;
    std::cout << "Adapter deviceID: " << properties.deviceID << std::endl;
    std::cout << "Adapter backend: " << properties.backendType << std::endl;
}

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
static WGPUAdapter request_adapter(WGPUInstance instance,
                                   WGPURequestAdapterOptions const* options)
{
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded   = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded
      = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
           char const* message, void* pUserData) {
            UserData* userData = (UserData*)pUserData;

            if (status == WGPURequestAdapterStatus_Success)
                userData->adapter = adapter;
            else
                std::cout << "Could not get WebGPU adapter: " << message
                          << std::endl;

            userData->requestEnded = true;
        };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(instance /* equivalent of navigator.gpu */,
                               options, onAdapterRequestEnded,
                               (void*)&userData);
#ifdef __EMSCRIPTEN__
    // In the Emscripten environment, the WebGPU adapter request is asynchronous
    while (!userData.requestEnded) emscripten_sleep(10);
#endif
    // In theory we should wait until onAdapterReady has been called, which
    // could take some time (what the 'await' keyword does in the JavaScript
    // code). In practice, we know that when the wgpuInstanceRequestAdapter()
    // function returns its callback has been called.
    ASSERT(userData.requestEnded);

    return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDevice(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
static WGPUDevice request_device(WGPUAdapter adapter,
                                 WGPUDeviceDescriptor const* descriptor)
{
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded
      = [](WGPURequestDeviceStatus status, WGPUDevice device,
           char const* message, void* pUserData) {
            UserData& userData = *reinterpret_cast<UserData*>(pUserData);
            if (status == WGPURequestDeviceStatus_Success) {
                userData.device = device;
            } else {
                std::cout << "Could not get WebGPU device: " << message
                          << std::endl;
            }
            userData.requestEnded = true;
        };

    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded,
                             (void*)&userData);

#ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) emscripten_sleep(10);
#endif

    ASSERT(userData.requestEnded);

    return userData.device;
}

static void on_device_error(WGPUErrorType type, char const* message,
                            void* /* pUserData */)
{
    log_error("Uncaptured device error: type %d (%s)", type, message);
#ifdef CHUGL_DEBUG
    exit(EXIT_FAILURE);
#endif
};

static bool createSwapChain(GraphicsContext* context, u32 width, u32 height)
{
    // ensure previous swap chain has been released
    ASSERT(context->swapChain == NULL);

    WGPUSwapChainDescriptor swap_chain_desc = {
        NULL,                              // nextInChain
        "The default swap chain",          // label
        WGPUTextureUsage_RenderAttachment, // usage
        context->swapChainFormat,          // format
        width,                             // width
        height,                            // height
        WGPUPresentMode_Fifo,              // presentMode (vsynced)
    };
    context->swapChain = wgpuDeviceCreateSwapChain(
      context->device, context->surface, &swap_chain_desc);

    if (!context->swapChain) return false;
    return true;
}

static void createDepthTexture(GraphicsContext* context, u32 width, u32 height)
{
    // Ensure that the depth texture is not already created or has been released
    ASSERT(context->depthTexture == NULL);
    ASSERT(context->depthTextureView == NULL);

    // Depth texture
    WGPUTextureDescriptor depthTextureDesc = {};
    // only support one format for now
    WGPUTextureFormat depthTextureFormat
      = WGPUTextureFormat_Depth24PlusStencil8;
    depthTextureDesc.usage         = WGPUTextureUsage_RenderAttachment;
    depthTextureDesc.dimension     = WGPUTextureDimension_2D;
    depthTextureDesc.size          = { width, height, 1 };
    depthTextureDesc.format        = depthTextureFormat;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount   = 1;
    context->depthTexture
      = wgpuDeviceCreateTexture(context->device, &depthTextureDesc);
    ASSERT(context->depthTexture != NULL);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depthTextureViewDesc = {};
    depthTextureViewDesc.format                    = depthTextureFormat;
    depthTextureViewDesc.dimension       = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.baseMipLevel    = 0;
    depthTextureViewDesc.mipLevelCount   = 1;
    depthTextureViewDesc.baseArrayLayer  = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.aspect          = WGPUTextureAspect_All;
    context->depthTextureView
      = wgpuTextureCreateView(context->depthTexture, &depthTextureViewDesc);
    ASSERT(context->depthTextureView != NULL);

    // defaults for render pass depth/stencil attachment
    context->depthStencilAttachment.view = context->depthTextureView;
    // The initial value of the depth buffer, meaning "far"
    context->depthStencilAttachment.depthClearValue = 1.0f;
    // Operation settings comparable to the color attachment
    context->depthStencilAttachment.depthLoadOp  = WGPULoadOp_Clear;
    context->depthStencilAttachment.depthStoreOp = WGPUStoreOp_Store;
    // we could turn off writing to the depth buffer globally here
    context->depthStencilAttachment.depthReadOnly = false;

    // Stencil setup, mandatory but unused
    context->depthStencilAttachment.stencilClearValue = 0;
#ifdef WEBGPU_BACKEND_DAWN
    context->depthStencilAttachment.stencilLoadOp  = WGPULoadOp_Undefined;
    context->depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Undefined;
#else
    context->depthStencilAttachment.stencilLoadOp  = WGPULoadOp_Clear;
    context->depthStencilAttachment.stencilStoreOp = WGPUStoreOp_Store;
#endif
    context->depthStencilAttachment.stencilReadOnly = false;
}

bool GraphicsContext::init(GraphicsContext* context, GLFWwindow* window)
{
    log_trace("initializing WebGPU context");
    ASSERT(context->instance == NULL);

#ifdef __EMSCRIPTEN__
    // See
    // https://github.com/emscripten-core/emscripten/blob/main/system/lib/webgpu/webgpu.cpp#L22
    // instance descriptor not implemented yet (as of 4/8/2024)
    // must pass nullptr instead
    context->instance = wgpuCreateInstance(NULL);
#else
    WGPUInstanceDescriptor instanceDesc = {};
    context->instance                   = wgpuCreateInstance(&instanceDesc);
#endif
    if (!context->instance) return false;
    log_trace("WebGPU instance created");

    context->surface = glfwGetWGPUSurface(context->instance, window);
    if (!context->surface) return false;
    // context->window = window;
    log_trace("WebGPU surface created");

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.compatibleSurface         = context->surface;
    adapterOpts.powerPreference           = WGPUPowerPreference_HighPerformance;
    // NULL,                                // nextInChain
    // context->surface,                    // compatibleSurface
    // WGPUPowerPreference_HighPerformance, // powerPreference
    // false                                // force fallback adapter
    context->adapter = request_adapter(context->instance, &adapterOpts);
    if (!context->adapter) return false;
    log_trace("adapter created");

    // TODO add device feature limits
    // simple: inspect adapter max limits and request max
    WGPUDeviceDescriptor deviceDescriptor = {
        NULL,                          // nextInChain
        "WebGPU-Renderer Device",      // label
        0,                             // requiredFeaturesCount
        NULL,                          // requiredFeatures
        NULL,                          // requiredLimits
        { NULL, "The default queue" }, // defaultQueue
        NULL,                          // deviceLostCallback,
        NULL                           // deviceLostUserdata
    };
    context->device = request_device(context->adapter, &deviceDescriptor);
    if (!context->device) return false;
    log_trace("device created");

    wgpuDeviceSetUncapturedErrorCallback(context->device, on_device_error,
                                         NULL /* pUserData */);

    context->queue = wgpuDeviceGetQueue(context->device);
    if (!context->queue) return false;

    int windowWidth = 1, windowHeight = 1;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    context->swapChainFormat
      = wgpuSurfaceGetPreferredFormat(context->surface, context->adapter);

    // Create the swap chain
    if (!createSwapChain(context, windowWidth, windowHeight)) return false;

    // Create depth texture and view
    createDepthTexture(context, windowWidth, windowHeight);

    // defaults for render pass color attachment
    context->colorAttachment = {};

#ifdef __EMSCRIPTEN__
    context->colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
    context->colorAttachment.loadOp     = WGPULoadOp_Clear;
    context->colorAttachment.storeOp    = WGPUStoreOp_Store;
    context->colorAttachment.clearValue = WGPUColor{ 0.0f, 0.0f, 0.0f, 1.0f };

    // render pass descriptor
    context->renderPassDesc.label                = "My render pass";
    context->renderPassDesc.colorAttachmentCount = 1;
    context->renderPassDesc.colorAttachments     = &context->colorAttachment;
    context->renderPassDesc.depthStencilAttachment
      = &context->depthStencilAttachment;

    return true;
}

WGPURenderPassEncoder GraphicsContext::prepareFrame(GraphicsContext* ctx)
{
    // get target texture view
    ctx->backbufferView = wgpuSwapChainGetCurrentTextureView(ctx->swapChain);
    ASSERT(ctx->backbufferView != NULL);
    ctx->colorAttachment.view = ctx->backbufferView;

    // initialize encoder
    WGPUCommandEncoderDescriptor encoderDesc = {};
    ctx->commandEncoder
      = wgpuDeviceCreateCommandEncoder(ctx->device, &encoderDesc);

    ctx->renderPassEncoder = wgpuCommandEncoderBeginRenderPass(
      ctx->commandEncoder, &ctx->renderPassDesc);

    return ctx->renderPassEncoder;
}

void GraphicsContext::presentFrame(GraphicsContext* ctx)
{
    wgpuRenderPassEncoderEnd(ctx->renderPassEncoder);
    wgpuRenderPassEncoderRelease(ctx->renderPassEncoder);

    // release texture view
    wgpuTextureViewRelease(ctx->backbufferView);

    // submit
    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    WGPUCommandBuffer command
      = wgpuCommandEncoderFinish(ctx->commandEncoder, &cmdBufferDescriptor);
    wgpuCommandEncoderRelease(ctx->commandEncoder);

    // Finally submit the command queue
    wgpuQueueSubmit(ctx->queue, 1, &command);
    wgpuCommandBufferRelease(command);

    // present
#ifndef __EMSCRIPTEN__
    wgpuSwapChainPresent(ctx->swapChain);
#endif
}

void GraphicsContext::resize(GraphicsContext* ctx, u32 width, u32 height)
{

    // terminate depth buffer
    WGPU_RELEASE_RESOURCE(TextureView, ctx->depthTextureView);
    WGPU_DESTROY_RESOURCE(Texture, ctx->depthTexture);
    WGPU_RELEASE_RESOURCE(Texture, ctx->depthTexture);

    // terminate swap chain
    WGPU_RELEASE_RESOURCE(SwapChain, ctx->swapChain);

    // recreate swap chain
    createSwapChain(ctx, width, height);
    // recreate depth texture
    createDepthTexture(ctx, width, height);
}

void GraphicsContext::release(GraphicsContext* ctx)
{
    // textures
    wgpuTextureViewRelease(ctx->depthTextureView);
    wgpuTextureDestroy(ctx->depthTexture);
    wgpuTextureRelease(ctx->depthTexture);

    wgpuSwapChainRelease(ctx->swapChain);
    wgpuDeviceRelease(ctx->device);
    wgpuAdapterRelease(ctx->adapter);
    wgpuInstanceRelease(ctx->instance);
    wgpuSurfaceRelease(ctx->surface);

    *ctx = {};
}

void VertexBuffer::init(GraphicsContext* ctx, VertexBuffer* buf,
                        u64 data_length, const f32* data, const char* label)
{
    // update description
    buf->desc.label = label;
    buf->desc.usage |= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    buf->desc.size             = sizeof(f32) * data_length;
    buf->desc.mappedAtCreation = false;

    buf->buf = wgpuDeviceCreateBuffer(ctx->device, &buf->desc);

    if (data)
        wgpuQueueWriteBuffer(ctx->queue, buf->buf, 0, data, buf->desc.size);
}

void IndexBuffer::init(GraphicsContext* ctx, IndexBuffer* buf, u64 data_length,
                       const u32* data, const char* label)
{
    // update description
    buf->desc.label = label;
    buf->desc.usage |= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    buf->desc.size             = sizeof(u32) * data_length;
    buf->desc.mappedAtCreation = false;

    buf->buf = wgpuDeviceCreateBuffer(ctx->device, &buf->desc);

    if (data)
        wgpuQueueWriteBuffer(ctx->queue, buf->buf, 0, data, buf->desc.size);
}

void VertexBufferLayout::init(VertexBufferLayout* layout, u8 attribute_count,
                              u32* attribute_strides)
{
    layout->attribute_count = attribute_count;
    WGPUVertexFormat format = WGPUVertexFormat_Undefined;

    for (u8 i = 0; i < attribute_count; i++) {
        // determine format
        switch (attribute_strides[i]) {
            case 1: format = WGPUVertexFormat_Float32; break;
            case 2: format = WGPUVertexFormat_Float32x2; break;
            case 3: format = WGPUVertexFormat_Float32x3; break;
            case 4: format = WGPUVertexFormat_Float32x4; break;
            default: format = WGPUVertexFormat_Undefined; break;
        }

        layout->attributes[i] = {
            format, // format
            0,      // offset
            i,      // shader location
        };

        layout->layouts[i] = {
            sizeof(f32) * attribute_strides[i], // arrayStride
            WGPUVertexStepMode_Vertex, // stepMode (TODO support instance)
            1,                         // attribute count
            layout->attributes + i,    // vertexAttribute
        };
    }
}

// Shaders ================================================================

void ShaderModule::init(GraphicsContext* ctx, ShaderModule* module,
                        const char* code, const char* label)
{
    // weird, WGPUShaderModuleDescriptor is the "base class" descriptor, and
    // WGSLDescriptor is the "derived" descriptor but the base class in this
    // case contains the "derived" class in its nextInChain field like OOP
    // inheritance flipped, where the parent contains the child data...
    module->wgsl_desc
      = { { nullptr, WGPUSType_ShaderModuleWGSLDescriptor }, // base class
          code };
    module->desc             = {};
    module->desc.label       = label;
    module->desc.nextInChain = &module->wgsl_desc.chain;

    module->module = wgpuDeviceCreateShaderModule(ctx->device, &module->desc);
}

void ShaderModule::release(ShaderModule* module)
{
    wgpuShaderModuleRelease(module->module);
}

// ============================================================================
// Blend State
// ============================================================================

static WGPUBlendState createBlendState(bool enableBlend)
{
    WGPUBlendComponent descriptor = {};
    descriptor.operation          = WGPUBlendOperation_Add;

    if (enableBlend) {
        descriptor.srcFactor = WGPUBlendFactor_SrcAlpha;
        descriptor.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    } else {
        descriptor.srcFactor = WGPUBlendFactor_One;
        descriptor.dstFactor = WGPUBlendFactor_Zero;
    }

    return {
        descriptor, // color
        descriptor, // alpha
    };
}

// ============================================================================
// Depth / Stencil State
// ============================================================================

static WGPUDepthStencilState createDepthStencilState(WGPUTextureFormat format,
                                                     bool enableDepthWrite)
{
    WGPUStencilFaceState stencil = {};
    stencil.compare              = WGPUCompareFunction_Always;
    stencil.failOp               = WGPUStencilOperation_Keep;
    stencil.depthFailOp          = WGPUStencilOperation_Keep;
    stencil.passOp               = WGPUStencilOperation_Keep;

    WGPUDepthStencilState depthStencilState = {};
    depthStencilState.depthWriteEnabled     = enableDepthWrite;
    depthStencilState.format                = format;
    // WGPUCompareFunction_LessEqual vs Less ?
    depthStencilState.depthCompare        = WGPUCompareFunction_Less;
    depthStencilState.stencilFront        = stencil;
    depthStencilState.stencilBack         = stencil;
    depthStencilState.stencilReadMask     = 0xFFFFFFFF;
    depthStencilState.stencilWriteMask    = 0xFFFFFFFF;
    depthStencilState.depthBias           = 0;
    depthStencilState.depthBiasSlopeScale = 0.0f;
    depthStencilState.depthBiasClamp      = 0.0f;

    return depthStencilState;
}

// ============================================================================
// Render Pipeline
// ============================================================================

static WGPUBindGroupLayout createBindGroupLayout(GraphicsContext* ctx,
                                                 u8 bindingNumber, u64 size)
{
    UNUSED_VAR(bindingNumber);

    // Per frame uniform buffer
    WGPUBindGroupLayoutEntry bindGroupLayout = {};
    // bindGroupLayout.binding                  = bindingNumber;
    bindGroupLayout.binding = 0;
    bindGroupLayout.visibility // always both for simplicity
      = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bindGroupLayout.buffer.type             = WGPUBufferBindingType_Uniform;
    bindGroupLayout.buffer.minBindingSize   = size;
    bindGroupLayout.buffer.hasDynamicOffset = false; // TODO

    // Create a bind group layout
    WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
    bindGroupLayoutDesc.entryCount                    = 1;
    bindGroupLayoutDesc.entries                       = &bindGroupLayout;
    return wgpuDeviceCreateBindGroupLayout(ctx->device, &bindGroupLayoutDesc);
}

// ============================================================================
// Bind Group
// ============================================================================

void BindGroup::init(GraphicsContext* ctx, BindGroup* bindGroup,
                     WGPUBindGroupLayout layout, u64 bufferSize)
{
    // for now hardcoded to only support uniform buffers
    // TODO: support textures, storage buffers, samplers, etc.
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size                 = bufferSize;
    bufferDesc.mappedAtCreation     = false;
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    bindGroup->uniformBuffer = wgpuDeviceCreateBuffer(ctx->device, &bufferDesc);

    // force only 1 @binding per @group
    WGPUBindGroupEntry binding = {};
    binding.binding            = 0; // @binding(0)
    binding.offset             = 0;
    binding.buffer
      = bindGroup->uniformBuffer; // only supports uniform buffers for now
    binding.size = bufferSize;

    // A bind group contains one or multiple bindings
    bindGroup->desc = {};
    bindGroup->desc.layout
      = layout; // TODO can get layout from renderpipeline via
                // WGPUProcRenderPipelineGetBindGroupLayout()
    bindGroup->desc.entries    = &binding;
    bindGroup->desc.entryCount = 1; // force 1 binding per group

    std::cout << "Creating bind group with layout " << layout << std::endl;

    bindGroup->bindGroup
      = wgpuDeviceCreateBindGroup(ctx->device, &bindGroup->desc);
    std::cout << "bindGroup: " << bindGroup->bindGroup << std::endl;
}

// ============================================================================
// Render Pipeline
// ============================================================================

void RenderPipeline::init(GraphicsContext* ctx, RenderPipeline* pipeline,
                          const char* vertexShaderCode,
                          const char* fragmentShaderCode)
{

    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology           = WGPUPrimitiveTopology_TriangleList;
    primitiveState.stripIndexFormat   = WGPUIndexFormat_Undefined;
    primitiveState.frontFace          = WGPUFrontFace_CCW;
    primitiveState.cullMode           = WGPUCullMode_None; // TODO: backface

    WGPUBlendState blendState             = createBlendState(true);
    WGPUColorTargetState colorTargetState = {};
    colorTargetState.format               = ctx->swapChainFormat;
    colorTargetState.blend                = &blendState;
    colorTargetState.writeMask            = WGPUColorWriteMask_All;

    WGPUDepthStencilState depth_stencil_state
      = createDepthStencilState(WGPUTextureFormat_Depth24PlusStencil8, true);

    // Setup shader module
    ShaderModule vertexShaderModule = {}, fragmentShaderModule = {};
    ShaderModule::init(ctx, &vertexShaderModule, vertexShaderCode,
                       "vertex shader");
    ShaderModule::init(ctx, &fragmentShaderModule, fragmentShaderCode,
                       "fragment shader");

    // for now hardcode attributes to 3:
    // position, normal, uv
    VertexBufferLayout vertexBufferLayout = {};
    u32 attributeStrides[]                = {
        3, // position
        3, // normal
        2, // uv
    };
    VertexBufferLayout::init(&vertexBufferLayout,
                             ARRAY_LENGTH(attributeStrides), attributeStrides);

    // vertex state
    WGPUVertexState vertexState = {};
    vertexState.bufferCount     = vertexBufferLayout.attribute_count;
    vertexState.buffers         = vertexBufferLayout.layouts;
    vertexState.module          = vertexShaderModule.module;
    vertexState.entryPoint      = VS_ENTRY_POINT;

    // fragment state
    WGPUFragmentState fragmentState = {};
    fragmentState.module            = fragmentShaderModule.module;
    fragmentState.entryPoint        = FS_ENTRY_POINT;
    fragmentState.targetCount       = 1;
    fragmentState.targets           = &colorTargetState;

    // multisample state
    WGPUMultisampleState multisampleState   = {};
    multisampleState.count                  = 1;
    multisampleState.mask                   = 0xFFFFFFFF;
    multisampleState.alphaToCoverageEnabled = false;

    // layout
    pipeline->bindGroupLayouts[PER_FRAME_GROUP]
      = createBindGroupLayout(ctx, PER_FRAME_GROUP, sizeof(FrameUniforms));

    // material layout
    {
        WGPUBindGroupLayoutEntry bindGroupLayouts[3];

        // Per material uniforms
        bindGroupLayouts[0]         = {};
        bindGroupLayouts[0].binding = 0;
        bindGroupLayouts[0].visibility // always both for simplicity
          = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
        bindGroupLayouts[0].buffer.type = WGPUBufferBindingType_Uniform;
        bindGroupLayouts[0].buffer.minBindingSize   = sizeof(MaterialUniforms);
        bindGroupLayouts[0].buffer.hasDynamicOffset = false;

        // per material texture
        bindGroupLayouts[1]                       = {};
        bindGroupLayouts[1].binding               = 1;
        bindGroupLayouts[1].visibility            = WGPUShaderStage_Fragment;
        bindGroupLayouts[1].texture.sampleType    = WGPUTextureSampleType_Float;
        bindGroupLayouts[1].texture.viewDimension = WGPUTextureViewDimension_2D;

        // per material sampler
        bindGroupLayouts[2]              = {};
        bindGroupLayouts[2].binding      = 2;
        bindGroupLayouts[2].visibility   = WGPUShaderStage_Fragment;
        bindGroupLayouts[2].sampler.type = WGPUSamplerBindingType_Filtering;

        // Create a bind group layout
        WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {};
        bindGroupLayoutDesc.entryCount = ARRAY_LENGTH(bindGroupLayouts);
        bindGroupLayoutDesc.entries    = bindGroupLayouts;
        pipeline->bindGroupLayouts[PER_MATERIAL_GROUP]
          = wgpuDeviceCreateBindGroupLayout(ctx->device, &bindGroupLayoutDesc);
    }

    pipeline->bindGroupLayouts[PER_DRAW_GROUP]
      = createBindGroupLayout(ctx, PER_DRAW_GROUP, sizeof(DrawUniforms));

    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount = ARRAY_LENGTH(pipeline->bindGroupLayouts);
    layoutDesc.bindGroupLayouts = pipeline->bindGroupLayouts; // one per @group
    WGPUPipelineLayout pipelineLayout
      = wgpuDeviceCreatePipelineLayout(ctx->device, &layoutDesc);

    // bind groups
    BindGroup::init(ctx, &pipeline->bindGroups[PER_FRAME_GROUP],
                    pipeline->bindGroupLayouts[PER_FRAME_GROUP],
                    sizeof(FrameUniforms));

    pipeline->desc              = {};
    pipeline->desc.label        = "render pipeline";
    pipeline->desc.layout       = pipelineLayout; // TODO
    pipeline->desc.primitive    = primitiveState;
    pipeline->desc.vertex       = vertexState;
    pipeline->desc.fragment     = &fragmentState;
    pipeline->desc.depthStencil = &depth_stencil_state;
    pipeline->desc.multisample  = multisampleState;

    pipeline->pipeline
      = wgpuDeviceCreateRenderPipeline(ctx->device, &pipeline->desc);

    ASSERT(pipeline->pipeline != NULL);

    // release wgpu resources
    ShaderModule::release(&vertexShaderModule);
    ShaderModule::release(&fragmentShaderModule);
}

void RenderPipeline::release(RenderPipeline* pipeline)
{
    wgpuRenderPipelineRelease(pipeline->pipeline);
}

// ============================================================================
// Depth Texture
// ============================================================================

void DepthTexture::init(GraphicsContext* ctx, DepthTexture* depthTexture,
                        WGPUTextureFormat format)
{
    // save format
    depthTexture->format = format;

    // Create the depth texture
    WGPUTextureDescriptor depthTextureDesc = {};
    depthTextureDesc.dimension             = WGPUTextureDimension_2D;
    depthTextureDesc.format                = format;
    depthTextureDesc.mipLevelCount         = 1;
    depthTextureDesc.sampleCount           = 1;
    // TODO: get size from GraphicsContext
    depthTextureDesc.size = { 640, 480, 1 };
    // also usage WGPUTextureUsage_Binding?
    depthTextureDesc.usage = WGPUTextureUsage_RenderAttachment;
    depthTexture->texture
      = wgpuDeviceCreateTexture(ctx->device, &depthTextureDesc);

    // Create the view of the depth texture manipulated by the rasterizer
    WGPUTextureViewDescriptor depthTextureViewDesc{};
    depthTextureViewDesc.label           = "Depth Texture View";
    depthTextureViewDesc.format          = format;
    depthTextureViewDesc.dimension       = WGPUTextureViewDimension_2D;
    depthTextureViewDesc.baseMipLevel    = 0;
    depthTextureViewDesc.mipLevelCount   = 1;
    depthTextureViewDesc.baseArrayLayer  = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.aspect          = WGPUTextureAspect_All;

    depthTexture->view
      = wgpuTextureCreateView(depthTexture->texture, &depthTextureViewDesc);
}

void DepthTexture::release(DepthTexture* depthTexture)
{
    wgpuTextureViewRelease(depthTexture->view);
    wgpuTextureDestroy(depthTexture->texture);
    wgpuTextureRelease(depthTexture->texture);
}

// ============================================================================
// MipMapGenerator (static)
// ============================================================================

#define NUMBER_OF_TEXTURE_FORMATS WGPUTextureFormat_ASTC12x12UnormSrgb // 94

/// @brief Determines the number of mip levels needed for a full mip chain
static u32 mipLevelCount(int width, int height)
{
    return (u32)(floor((float)(log2(MAX(width, height))))) + 1;
}

// TODO make part of GraphicsContext and cleanup
struct MipMapGenerator {
    GraphicsContext* ctx;
    WGPUSampler sampler;

    // Pipeline for every texture format used.
    // TODO: can layout be shared?
    WGPUBindGroupLayout pipeline_layouts[(u32)NUMBER_OF_TEXTURE_FORMATS];
    WGPURenderPipeline pipelines[(u32)NUMBER_OF_TEXTURE_FORMATS];
    bool active_pipelines[(u32)NUMBER_OF_TEXTURE_FORMATS];

    // Vertex state and Fragment state are shared between all pipelines
    WGPUVertexState vertexState;
    WGPUFragmentState fragmentState;

    static void init(GraphicsContext* ctx, MipMapGenerator* generator);
    static WGPURenderPipeline getPipeline(MipMapGenerator* generator,
                                          WGPUTextureFormat format);
    static WGPUTexture generate(MipMapGenerator* generator, WGPUTexture texture,
                                WGPUTextureDescriptor* texture_desc);
    static void release(MipMapGenerator* generator);
};

static MipMapGenerator mipMapGenerator = {};

void MipMapGenerator::init(GraphicsContext* ctx, MipMapGenerator* generator)
{
    generator->ctx = ctx;

    // Create sampler
    WGPUSamplerDescriptor sampler_desc = {};
    sampler_desc.label                 = "mip map sampler";
    sampler_desc.addressModeU          = WGPUAddressMode_ClampToEdge;
    sampler_desc.addressModeV          = WGPUAddressMode_ClampToEdge;
    sampler_desc.addressModeW          = WGPUAddressMode_ClampToEdge;
    sampler_desc.minFilter             = WGPUFilterMode_Linear;
    sampler_desc.magFilter             = WGPUFilterMode_Nearest;
    sampler_desc.mipmapFilter          = WGPUMipmapFilterMode_Nearest;
    sampler_desc.lodMinClamp           = 0.0f;
    sampler_desc.lodMaxClamp           = 1.0f;
    sampler_desc.maxAnisotropy         = 1;
    generator->sampler = wgpuDeviceCreateSampler(ctx->device, &sampler_desc);
}

void MipMapGenerator::release(MipMapGenerator* generator)
{
    // release sampler
    wgpuSamplerReference(generator->sampler);

    // release pipelines
    for (uint32_t i = 0; i < (uint32_t)NUMBER_OF_TEXTURE_FORMATS; ++i) {
        if (generator->active_pipelines[i]) {
            wgpuRenderPipelineRelease(generator->pipelines[i]);
            generator->active_pipelines[i] = false;
        }
    }

    // release shaders
    // if (generator->vertexState.module != NULL)
    wgpuShaderModuleRelease(generator->vertexState.module);
    // if (generator->fragmentState.module != NULL)
    wgpuShaderModuleRelease(generator->fragmentState.module);
}

WGPURenderPipeline MipMapGenerator::getPipeline(MipMapGenerator* generator,
                                                WGPUTextureFormat format)
{
    u32 pipeline_index = (u32)format;
    ASSERT(pipeline_index < (u32)NUMBER_OF_TEXTURE_FORMATS)
    if (generator->active_pipelines[pipeline_index])
        return generator->pipelines[pipeline_index];

    // Create pipeline if it doesn't exist
    GraphicsContext* ctx = generator->ctx;

    // Primitive state
    WGPUPrimitiveState primitiveStateDesc = {};
    primitiveStateDesc.topology           = WGPUPrimitiveTopology_TriangleStrip;
    primitiveStateDesc.stripIndexFormat   = WGPUIndexFormat_Uint32;
    primitiveStateDesc.frontFace          = WGPUFrontFace_CCW;
    primitiveStateDesc.cullMode           = WGPUCullMode_None;

    // Color target state
    WGPUBlendState blend_state                   = createBlendState(false);
    WGPUColorTargetState color_target_state_desc = {};
    color_target_state_desc.format               = format;
    color_target_state_desc.blend                = &blend_state;
    color_target_state_desc.writeMask            = WGPUColorWriteMask_All;

    // Vertex state and Fragment state are shared between all pipelines, so
    // only create once.
    if (!generator->vertexState.module || !generator->fragmentState.module) {

        ShaderModule vertexShaderModule = {}, fragmentShaderModule = {};
        ShaderModule::init(ctx, &vertexShaderModule, mipMapShader,
                           "mipmap vertex shader");
        ShaderModule::init(ctx, &fragmentShaderModule, mipMapShader,
                           "mipmap fragment shader");
        // vertex state
        generator->vertexState             = {};
        generator->vertexState.bufferCount = 0;
        generator->vertexState.buffers     = NULL;
        generator->vertexState.module      = vertexShaderModule.module;
        generator->vertexState.entryPoint  = VS_ENTRY_POINT;

        // fragment state
        generator->fragmentState             = {};
        generator->fragmentState.module      = fragmentShaderModule.module;
        generator->fragmentState.entryPoint  = FS_ENTRY_POINT;
        generator->fragmentState.targetCount = 1;
        generator->fragmentState.targets     = &color_target_state_desc;

        // don't release shader modules here, they are released in
        // MipMapGenerator_release shader modules need to be saved for creating
        // other pipelines
    }

    // Multisample state
    WGPUMultisampleState multisampleState   = {};
    multisampleState.count                  = 1;
    multisampleState.mask                   = 0xFFFFFFFF;
    multisampleState.alphaToCoverageEnabled = false;

    WGPURenderPipelineDescriptor pipelineDesc = {};
    // layout: defaults to `auto`
    pipelineDesc.label       = "mipmap blit render pipeline";
    pipelineDesc.primitive   = primitiveStateDesc;
    pipelineDesc.vertex      = generator->vertexState;
    pipelineDesc.fragment    = &generator->fragmentState;
    pipelineDesc.multisample = multisampleState;

    // Create rendering pipeline using the specified states
    generator->pipelines[pipeline_index]
      = wgpuDeviceCreateRenderPipeline(ctx->device, &pipelineDesc);
    ASSERT(generator->pipelines[pipeline_index] != NULL);

    // Store the bind group layout of the created pipeline
    // TODO: can probably re-use a single bindgroup for all pipelines
    generator->pipeline_layouts[pipeline_index]
      = wgpuRenderPipelineGetBindGroupLayout(
        generator->pipelines[pipeline_index], 0);
    ASSERT(generator->pipeline_layouts[pipeline_index] != NULL)

    // Update active pipeline state
    generator->active_pipelines[pipeline_index] = true;

    return generator->pipelines[pipeline_index];
}

WGPUTexture MipMapGenerator::generate(MipMapGenerator* generator,
                                      WGPUTexture texture,
                                      WGPUTextureDescriptor* texture_desc)
{
    WGPURenderPipeline pipeline
      = MipMapGenerator::getPipeline(generator, texture_desc->format);

    log_trace("Generating %d mip levels for texture %s",
              texture_desc->mipLevelCount, texture_desc->label);

    if (texture_desc->dimension == WGPUTextureDimension_3D
        || texture_desc->dimension == WGPUTextureDimension_1D) {
        log_error(
          "Generating mipmaps for non-2d textures is currently unsupported!" //
        );
        return NULL;
    }

    GraphicsContext* ctx        = generator->ctx;
    WGPUTexture mip_texture     = texture;
    const u32 array_layer_count = texture_desc->size.depthOrArrayLayers > 0 ?
                                    texture_desc->size.depthOrArrayLayers :
                                    1; // Only valid for 2D textures.
    const u32 mip_level_count   = texture_desc->mipLevelCount;

    WGPUExtent3D mip_level_size = {
        (u32)ceil(texture_desc->size.width / 2.0f),
        (u32)ceil(texture_desc->size.height / 2.0f),
        array_layer_count,
    };

    // If the texture was created with RENDER_ATTACHMENT usage we can render
    // directly between mip levels.
    bool render_to_source
      = texture_desc->usage & WGPUTextureUsage_RenderAttachment;
    if (!render_to_source) {
        // Otherwise we have to use a separate texture to render into. It can be
        // one mip level smaller than the source texture, since we already have
        // the top level.
        WGPUTextureDescriptor mip_texture_desc = {};
        mip_texture_desc.size                  = mip_level_size;
        mip_texture_desc.format                = texture_desc->format;
        mip_texture_desc.usage                 = WGPUTextureUsage_CopySrc
                                 | WGPUTextureUsage_TextureBinding
                                 | WGPUTextureUsage_RenderAttachment;
        mip_texture_desc.dimension     = WGPUTextureDimension_2D;
        mip_texture_desc.mipLevelCount = texture_desc->mipLevelCount - 1;
        mip_texture_desc.sampleCount   = 1;

        mip_texture = wgpuDeviceCreateTexture(ctx->device, &mip_texture_desc);
        ASSERT(mip_texture != NULL);
    }

    WGPUCommandEncoder cmd_encoder
      = wgpuDeviceCreateCommandEncoder(ctx->device, NULL);
    u32 pipeline_index = (u32)texture_desc->format;
    WGPUBindGroupLayout bind_group_layout
      = generator->pipeline_layouts[pipeline_index];

    const u32 views_count  = array_layer_count * mip_level_count;
    WGPUTextureView* views = ALLOCATE_COUNT(WGPUTextureView, views_count);

    const u32 bind_group_count = array_layer_count * (mip_level_count - 1);
    WGPUBindGroup* bind_groups
      = ALLOCATE_COUNT(WGPUBindGroup, bind_group_count);

    WGPUTextureViewDescriptor viewDesc = {};
    viewDesc.label                     = "src_view";
    viewDesc.aspect                    = WGPUTextureAspect_All;
    viewDesc.baseMipLevel              = 0;
    viewDesc.mipLevelCount             = 1;
    viewDesc.dimension                 = WGPUTextureViewDimension_2D;
    viewDesc.baseArrayLayer            = 0; // updated in loop
    viewDesc.arrayLayerCount           = 1;

    for (u32 array_layer = 0; array_layer < array_layer_count; ++array_layer) {
        u32 view_index = array_layer * mip_level_count;

        viewDesc.baseArrayLayer = array_layer;
        views[view_index]       = wgpuTextureCreateView(texture, &viewDesc);

        u32 dst_mip_level = render_to_source ? 1 : 0;
        for (u32 i = 1; i < texture_desc->mipLevelCount; ++i) {
            const uint32_t target_mip = view_index + i;

            WGPUTextureViewDescriptor mipViewDesc = {};
            mipViewDesc.label                     = "dst_view";
            mipViewDesc.aspect                    = WGPUTextureAspect_All;
            mipViewDesc.baseMipLevel              = dst_mip_level++;
            mipViewDesc.mipLevelCount             = 1;
            mipViewDesc.dimension                 = WGPUTextureViewDimension_2D;
            mipViewDesc.baseArrayLayer            = array_layer;
            mipViewDesc.arrayLayerCount           = 1;

            views[target_mip]
              = wgpuTextureCreateView(mip_texture, &mipViewDesc);

            WGPURenderPassColorAttachment colorAttachmentDesc = {};
            colorAttachmentDesc.view = views[target_mip];
#ifdef __EMSCRIPTEN__
            // depthSlice is new in the new header, not yet supported in
            // wgpu-native. See
            // https://github.com/eliemichel/WebGPU-distribution/issues/14
            colorAttachmentDesc.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
            colorAttachmentDesc.resolveTarget = NULL;
            colorAttachmentDesc.loadOp        = WGPULoadOp_Clear;
            colorAttachmentDesc.storeOp       = WGPUStoreOp_Store;
            colorAttachmentDesc.clearValue    = { 0.0f, 0.0f, 0.0f, 0.0f };

            WGPURenderPassDescriptor render_pass_desc = {};
            render_pass_desc.colorAttachmentCount     = 1;
            render_pass_desc.colorAttachments         = &colorAttachmentDesc;
            render_pass_desc.depthStencilAttachment   = NULL;

            WGPURenderPassEncoder pass_encoder
              = wgpuCommandEncoderBeginRenderPass(cmd_encoder,
                                                  &render_pass_desc);

            // initialize bind group entries
            WGPUBindGroupEntry bg_entries[2];

            // sampler bind group
            // TODO share sampler across calls
            bg_entries[0]         = {};
            bg_entries[0].binding = 0;
            bg_entries[0].sampler = generator->sampler;

            // source texture bind group
            bg_entries[1]             = {};
            bg_entries[1].binding     = 1;
            bg_entries[1].textureView = views[target_mip - 1];

            WGPUBindGroupDescriptor bg_desc = {};
            bg_desc.layout                  = bind_group_layout;
            bg_desc.entryCount              = ARRAY_LENGTH(bg_entries);
            bg_desc.entries                 = bg_entries;

            uint32_t bind_group_index
              = array_layer * (mip_level_count - 1) + i - 1;
            bind_groups[bind_group_index]
              = wgpuDeviceCreateBindGroup(ctx->device, &bg_desc);

            wgpuRenderPassEncoderSetPipeline(pass_encoder, pipeline);
            wgpuRenderPassEncoderSetBindGroup(
              pass_encoder, 0, bind_groups[bind_group_index], 0, NULL);
            wgpuRenderPassEncoderDraw(pass_encoder, 3, 1, 0, 0);
            wgpuRenderPassEncoderEnd(pass_encoder);

            WGPU_RELEASE_RESOURCE(RenderPassEncoder, pass_encoder)
        }
    }

    // If we didn't render to the source texture, finish by copying the mip
    // results from the temporary mipmap texture to the source.
    if (!render_to_source) {
        for (u32 i = 1; i < texture_desc->mipLevelCount; ++i) {

            // log_debug("Copying to mip level %d with sizes %d, %d\n", i,
            //           mip_level_size.width, mip_level_size.height);

            WGPUImageCopyTexture mipCopySrc = {};
            mipCopySrc.texture              = mip_texture;
            mipCopySrc.mipLevel             = i - 1;

            WGPUImageCopyTexture mipCopyDst = {};
            mipCopyDst.texture              = texture;
            mipCopyDst.mipLevel             = i;

            wgpuCommandEncoderCopyTextureToTexture(
              cmd_encoder, &mipCopySrc, &mipCopyDst, &mip_level_size //
            );

            // Turns out wgpu uses floor not ceil to determine mip size
            // mip_level_size.width  = ceil(mip_level_size.width / 2.0f);
            // mip_level_size.height = ceil(mip_level_size.height / 2.0f);
            mip_level_size.width  = floor(mip_level_size.width / 2.0f);
            mip_level_size.height = floor(mip_level_size.height / 2.0f);
        }
    }

    WGPUCommandBuffer command_buffer
      = wgpuCommandEncoderFinish(cmd_encoder, NULL);
    ASSERT(command_buffer != NULL);
    WGPU_RELEASE_RESOURCE(CommandEncoder, cmd_encoder)

    // Sumbit commmand buffer
    wgpuQueueSubmit(ctx->queue, 1, &command_buffer);

    { // cleanup
        WGPU_RELEASE_RESOURCE(CommandBuffer, command_buffer)

        if (!render_to_source) {
            WGPU_RELEASE_RESOURCE(Texture, mip_texture);
        }

        for (uint32_t i = 0; i < views_count; ++i) {
            WGPU_RELEASE_RESOURCE(TextureView, views[i]);
        }
        FREE_ARRAY(WGPUTextureView, views, views_count);

        for (uint32_t i = 0; i < bind_group_count; ++i) {
            WGPU_RELEASE_RESOURCE(BindGroup, bind_groups[i]);
        }
        FREE_ARRAY(WGPUBindGroup, bind_groups, bind_group_count);
    }

    return texture;
}

// ============================================================================
// Texture
// ============================================================================

void Texture::initFromFile(GraphicsContext* ctx, Texture* texture,
                           const char* filename, bool genMipMaps)
{
    ASSERT(texture->texture == NULL);

    i32 width = 0, height = 0;
    // Force loading 4 channel images to 3 channel by stb becasue Dawn
    // doesn't support 3 channel formats currently. The group is discussing
    // on whether webgpu shoud support 3 channel format.
    // https://github.com/gpuweb/gpuweb/issues/66#issuecomment-410021505
    i32 read_comps    = 0;
    i32 desired_comps = STBI_rgb_alpha; // force 4 channels

    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixel_data = stbi_load(filename,     //
                                    &width,       //
                                    &height,      //
                                    &read_comps,  //
                                    desired_comps //
    );

    if (pixel_data == NULL) {
        log_error("Couldn't load '%s'\n", filename);

        log_error("Reason: %s\n", stbi_failure_reason());
        return;
    } else {
        log_debug("Loaded image %s (%d, %d, %d / %d)\n", filename, width,
                  height, read_comps, desired_comps);
    }

    // save texture info
    texture->width  = width;
    texture->height = height;
    texture->depth  = 1;

    // default always gen mipmaps
    texture->mip_level_count = genMipMaps ? mipLevelCount(width, height) : 1u;

    // TODO: support compressed texture formats
    texture->format    = WGPUTextureFormat_RGBA8Unorm;
    texture->dimension = WGPUTextureDimension_2D;

    // create texture
    WGPUTextureDescriptor textureDesc = {};
    // render attachment usage is used for mip map generation
    textureDesc.usage
      = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
    // | WGPUTextureUsage_RenderAttachment;
    textureDesc.dimension       = texture->dimension;
    textureDesc.size            = { (u32)width, (u32)height, 1 };
    textureDesc.format          = texture->format;
    textureDesc.mipLevelCount   = texture->mip_level_count;
    textureDesc.sampleCount     = 1;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats     = NULL;
    textureDesc.label           = filename;

    texture->texture = wgpuDeviceCreateTexture(ctx->device, &textureDesc);
    ASSERT(texture->texture != NULL);

    // write texture data
    {
        WGPUImageCopyTexture destination = {};
        destination.texture              = texture->texture;
        destination.mipLevel             = 0;
        destination.origin               = {
            0,
            0,
            0,
        }; // equivalent of the offset argument of Queue::writeBuffer
        destination.aspect = WGPUTextureAspect_All; // only relevant for
                                                    // depth/Stencil textures

        WGPUTextureDataLayout source = {};
        source.offset       = 0; // where to start reading from the cpu buffer
        source.bytesPerRow  = desired_comps * textureDesc.size.width;
        source.rowsPerImage = textureDesc.size.height;

        const u64 dataSize = textureDesc.size.width * textureDesc.size.height
                             * textureDesc.size.depthOrArrayLayers
                             * desired_comps;

        wgpuQueueWriteTexture(ctx->queue, &destination, pixel_data, dataSize,
                              &source, &textureDesc.size);
    }

    // generate mipmaps
    if (genMipMaps) {
        if (mipMapGenerator.ctx == NULL)
            MipMapGenerator::init(ctx, &mipMapGenerator);

        // generate mipmaps
        MipMapGenerator::generate(&mipMapGenerator, texture->texture,
                                  &textureDesc);
    }

    // free pixel data
    stbi_image_free(pixel_data);

    /* Create the texture view */
    WGPUTextureViewDescriptor textureViewDesc = {};
    textureViewDesc.format                    = textureDesc.format;
    textureViewDesc.dimension                 = WGPUTextureViewDimension_2D;
    textureViewDesc.baseMipLevel              = 0;
    textureViewDesc.mipLevelCount             = textureDesc.mipLevelCount;
    textureViewDesc.baseArrayLayer            = 0;
    textureViewDesc.arrayLayerCount           = 1;
    textureViewDesc.aspect                    = WGPUTextureAspect_All;
    texture->view = wgpuTextureCreateView(texture->texture, &textureViewDesc);

    /* Create the texture sampler */
    WGPUSamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU          = WGPUAddressMode_Repeat;
    samplerDesc.addressModeV          = WGPUAddressMode_Repeat;
    samplerDesc.addressModeW          = WGPUAddressMode_Repeat;

    // TODO: make filtering configurable
    samplerDesc.minFilter    = WGPUFilterMode_Linear;
    samplerDesc.magFilter    = WGPUFilterMode_Linear;
    samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    // samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Nearest;

    samplerDesc.lodMinClamp   = 0.0f;
    samplerDesc.lodMaxClamp   = (f32)texture->mip_level_count;
    samplerDesc.maxAnisotropy = 1; // TODO: try max of 16

    texture->sampler = wgpuDeviceCreateSampler(ctx->device, &samplerDesc);
};

void Texture::release(Texture* texture)
{
    // release textureview
    WGPU_RELEASE_RESOURCE(TextureView, texture->view);

    // release texture
    WGPU_DESTROY_RESOURCE(Texture, texture->texture)
    WGPU_RELEASE_RESOURCE(Texture, texture->texture);

    // release sampler
    WGPU_RELEASE_RESOURCE(Sampler, texture->sampler)
}

// ============================================================================
// Material
// ============================================================================

void Material::init(GraphicsContext* ctx, Material* material,
                    RenderPipeline* pipeline, Texture* texture)
{
    ASSERT(material->bindGroup == NULL);
    ASSERT(texture != NULL);

    material->texture = texture;

    // uniform buffer
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size
      = sizeof(MaterialUniforms); // TODO: support multiple materials
    bufferDesc.mappedAtCreation = false;
    bufferDesc.usage        = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
    material->uniformBuffer = wgpuDeviceCreateBuffer(ctx->device, &bufferDesc);

    // build bind group entries
    {
        // uniform buffer bindgroup entry
        WGPUBindGroupEntry* uniformBinding = &material->entries[0];
        *uniformBinding                    = {};
        uniformBinding->binding            = 0; // @binding(0)
        uniformBinding->offset             = 0;
        uniformBinding->buffer             = material->uniformBuffer;
        uniformBinding->size               = bufferDesc.size;

        // texture bindgroup entry
        WGPUBindGroupEntry* textureBinding = &material->entries[1];
        *textureBinding                    = {};
        textureBinding->binding            = 1; // @binding(1)
        textureBinding->textureView        = texture->view;

        // sampler bindgroup entry
        WGPUBindGroupEntry* samplerBinding = &material->entries[2];
        *samplerBinding                    = {};
        samplerBinding->binding            = 2; // @binding(2)
        samplerBinding->sampler            = texture->sampler;
    }

    // A bind group contains one or multiple bindings
    material->desc            = {};
    material->desc.layout     = pipeline->bindGroupLayouts[PER_MATERIAL_GROUP];
    material->desc.entries    = material->entries;
    material->desc.entryCount = ARRAY_LENGTH(material->entries);
    ASSERT(material->desc.entryCount == 3);

    material->bindGroup
      = wgpuDeviceCreateBindGroup(ctx->device, &material->desc);
}

/// @brief Bind a texture to a material (replaces previous texture)
void Material::setTexture(GraphicsContext* ctx, Material* material,
                          Texture* texture)
{
    // don't release previous texture as it may be used by other materials

    // set new texture
    material->texture = texture;

    // update bind group entries
    {
        // uniform buffer bindgroup entry
        // WGPUBindGroupEntry* uniformBinding = &material->entries[0];
        // *uniformBinding = {};
        // uniformBinding->binding            = 0; // @binding(0)
        // uniformBinding->offset             = 0;
        // uniformBinding->buffer             = material->uniformBuffer;
        // uniformBinding->size               = bufferDesc.size;

        // texture bindgroup entry
        material->entries[1].textureView = texture->view;

        // sampler bindgroup entry
        material->entries[2].sampler = texture->sampler;
    }

    // release old bindgroup
    wgpuBindGroupRelease(material->bindGroup);

    // create new bindgroup
    material->bindGroup
      = wgpuDeviceCreateBindGroup(ctx->device, &material->desc);
}

void Material::release(Material* material)
{
    wgpuBindGroupRelease(material->bindGroup);

    // release buffer (TODO create uniform buffer struct)
    wgpuBufferDestroy(material->uniformBuffer);
    wgpuBufferRelease(material->uniformBuffer);
}
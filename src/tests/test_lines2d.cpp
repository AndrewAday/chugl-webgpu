#include "test_base.h"

#include "core/log.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_wgpu.h>
#include <imgui/imgui.h>

static GraphicsContext* gctx = NULL;
static GLFWwindow* window    = NULL;

// Vertex
// position
// Normals
// UVs

struct Uniforms {
    glm::mat4x4 world;         // at byte offset 0
    glm::mat4x4 view;          // at byte offset 64
    glm::mat4x4 proj;          // at byte offset 128
    float screen_width;        // at byte offset 192
    float screen_height;       // at byte offset 196
    uint32_t line_point_count; // at byte offset 200
    float line_width = 0.01f;
};

static const char* lines2D_shader = R"glsl(

    struct Uniforms {
        world           : mat4x4<f32>,
        view            : mat4x4<f32>,
        proj            : mat4x4<f32>,
        screen_width    : f32,
        screen_height   : f32,
        line_point_count: u32, // number of line points (half the number of vertices in the line strip)
        line_width      : f32,
    }

    struct PositionStorage {
        positionCount : u32,
        positions : array<f32>,
    }

    @group(0) @binding(0) var<uniform> uniforms : Uniforms;
    @group(0) @binding(1) var<storage, read> positions : array<f32>;
    // @binding(2) @group(0) var<storage, read> colors : U32s;
    // @binding(3) @group(0) var<storage, read> indices : U32s;

    struct VertexOutput {
        @builtin(position) position : vec4<f32>,
        @location(0) color : vec4<f32>
    }

    @vertex
    fn vs_main(@builtin(vertex_index) vertex_id : u32) -> VertexOutput {

        var output : VertexOutput;

        var pos_idx = (vertex_id / 2u) + 1u; // add 1 to skip sentinel start point
        var this_pos = vec2f(
            positions[3u * pos_idx + 0u],  // x
            positions[3u * pos_idx + 1u]   // y
        );
        var pos = this_pos;

        let half_width = uniforms.line_width * 0.5; 

        // get even/odd (odd vertices are expanded down, even vertices are expanded up)
        var orientation : f32 = 0.0;
        if (vertex_id % 2u == 0u) {
            orientation = 1.0;
        } else {
            orientation = -1.0;
        }

        // are we the first endpoint?
        // if (vertex_id / 2u == 0u) {
        //   var next_pos = vec2f(
        //     positions[3u * (pos_idx + 1u) + 0u],  // x
        //     positions[3u * (pos_idx + 1u) + 1u],  // y
        //   );

        //   var line_seg_dir = normalize(next_pos - this_pos);
        //   var perp_dir = orientation * vec2f(-line_seg_dir.y, line_seg_dir.x);

        //   // adjust position
        //   pos += half_width * perp_dir;

        //   // color debug
        //   output.color = vec4<f32>(1.0, 0.0, 0.0, 1.0);
        // } 
        // else if (vertex_id / 2u == uniforms.line_point_count - 1u) {
        //     // last endpoint
        //     var prev_pos = vec2f(
        //         positions[3u * (pos_idx - 1u) + 0u],  // x
        //         positions[3u * (pos_idx - 1u) + 1u],  // y
        //     );

        //     var line_seg_dir = normalize(this_pos - prev_pos);
        //     var perp_dir = orientation * vec2f(-line_seg_dir.y, line_seg_dir.x);

        //     // adjust position
        //     pos += half_width * perp_dir;

        //     // color debug
        //     output.color = vec4<f32>(0.0, 0.0, 1.0, 1.0);
        // } else {
            // middle points
            var prev_pos = vec2f(
                positions[3u * (pos_idx - 1u) + 0u],  // x
                positions[3u * (pos_idx - 1u) + 1u],  // y
            );
            var next_pos = vec2f(
                positions[3u * (pos_idx + 1u) + 0u],  // x
                positions[3u * (pos_idx + 1u) + 1u],  // y
            );

            var prev_dir = normalize(this_pos - prev_pos);
            var next_dir = normalize(next_pos - this_pos);
            var prev_dir_perp = orientation * vec2f(-prev_dir.y, prev_dir.x);
            var next_dir_perp = orientation * vec2f(-next_dir.y, next_dir.x);

            var miter_dir = normalize(prev_dir_perp + next_dir_perp);
            var miter_length = half_width / dot(miter_dir, prev_dir_perp);

            // adjust position
            pos += miter_length * miter_dir;

            // color debug
            output.color = vec4<f32>(0.0, 1.0, 0.0, 1.0);
        // }

        var position = vec4f(pos, 0.0, 1.0);
        position = uniforms.proj * uniforms.view * uniforms.world * position;

        // var color_u32 = colors.values[vertex.vertexID];
        // var color = vec4<f32>(
        //     f32((color_u32 >>  0u) & 0xFFu) / 255.0,
        //     f32((color_u32 >>  8u) & 0xFFu) / 255.0,
        //     f32((color_u32 >> 16u) & 0xFFu) / 255.0,
        //     f32((color_u32 >> 24u) & 0xFFu) / 255.0,
        // );

        output.position = position;

        return output;
    }


    struct FragmentInput {
        @location(0) color : vec4<f32>,
        @builtin(front_facing) is_front_facing : bool
    }

    struct FragmentOutput {
        @location(0) color : vec4<f32>
    }

    @fragment
    fn fs_main(fragment : FragmentInput) -> FragmentOutput {
        var output : FragmentOutput;
        output.color = fragment.color;

        // if (fragment.is_front_facing) {
        //     output.color = vec4<f32>(1.0, 0.0, 0.0, 1.0);
        // } else {
        //     output.color = vec4<f32>(0.0, 1.0, 0.0, 1.0);
        // }

        return output;
    }
)glsl";

// renderer state
static WGPURenderPipeline lines2D_pipeline = NULL;
static WGPUBuffer lines2d_uniform_buffer   = NULL;
static WGPUBuffer lines2d_positions        = NULL;
static WGPUBindGroup lines2d_bind_group    = NULL;

#define MAX_LINE_POINTS 1024
static glm::vec3 line_points[MAX_LINE_POINTS] = {};
static size_t num_points                      = 3;
static float line_width                       = 0.01f;
static bool line_loop                         = false;

static void _Test_Lines2D_onInit(GraphicsContext* ctx, GLFWwindow* w)
{
    line_points[1] = glm::vec3(-0.5f, -0.5f, 0.0f);
    line_points[2] = glm::vec3(0.0f, 0.5f, 0.0f);
    line_points[3] = glm::vec3(0.5f, -0.5f, 0.0f);

    gctx   = ctx;
    window = w;

    // init render pipelinetrue
    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology           = WGPUPrimitiveTopology_TriangleStrip;
    // primitiveState.topology         = WGPUPrimitiveTopology_LineStrip;
    primitiveState.stripIndexFormat = WGPUIndexFormat_Uint32; // always 32bit
    primitiveState.frontFace        = WGPUFrontFace_CCW;
    primitiveState.cullMode         = WGPUCullMode_None;

    WGPUBlendComponent blend_component
      = { WGPUBlendOperation_Add, WGPUBlendFactor_SrcAlpha,
          WGPUBlendFactor_OneMinusSrcAlpha };
    WGPUBlendState blendState = { blend_component, blend_component };

    WGPUColorTargetState colorTargetState = {};
    colorTargetState.format               = ctx->swapChainFormat;
    colorTargetState.blend                = &blendState;
    colorTargetState.writeMask            = WGPUColorWriteMask_All;

    WGPUDepthStencilState depth_stencil_state
      = G_createDepthStencilState(WGPUTextureFormat_Depth24PlusStencil8, true);

    // Setup shader module
    WGPUShaderModule vertexShaderModule
      = G_createShaderModule(gctx, lines2D_shader, "vertex shader");
    WGPUShaderModule fragmentShaderModule
      = G_createShaderModule(gctx, lines2D_shader, "frag shader");
    defer({
        wgpuShaderModuleRelease(vertexShaderModule);
        wgpuShaderModuleRelease(fragmentShaderModule);
        vertexShaderModule   = NULL;
        fragmentShaderModule = NULL;
    });

    // for now hardcode attributes to 3:
    // position, normal, uv
    // VertexBufferLayout vertexBufferLayout = {};
    // u32 attributeStrides[]                = {
    //     3, // position
    //     3, // normal
    //     2, // uv
    //     4, // tangent
    // };
    // VertexBufferLayout::init(&vertexBufferLayout,
    //                          ARRAY_LENGTH(attributeStrides),
    //                          attributeStrides);

    // vertex state
    WGPUVertexState vertexState = {};
    vertexState.bufferCount
      = 0; // programmable vertex pulling, no vertex buffers
    vertexState.buffers    = NULL;
    vertexState.module     = vertexShaderModule;
    vertexState.entryPoint = "vs_main";

    // fragment state
    WGPUFragmentState fragmentState = {};
    fragmentState.module            = fragmentShaderModule;
    fragmentState.entryPoint        = "fs_main";
    fragmentState.targetCount       = 1;
    fragmentState.targets           = &colorTargetState;

    // multisample state
    WGPUMultisampleState multisampleState = G_createMultisampleState(1);

    WGPURenderPipelineDescriptor pipeline_desc = {};
    pipeline_desc.layout                       = NULL; // auto
    pipeline_desc.primitive                    = primitiveState;
    pipeline_desc.vertex                       = vertexState;
    pipeline_desc.fragment                     = &fragmentState;
    pipeline_desc.depthStencil                 = &depth_stencil_state;
    pipeline_desc.multisample                  = multisampleState;

    lines2D_pipeline
      = wgpuDeviceCreateRenderPipeline(ctx->device, &pipeline_desc);
    ASSERT(lines2D_pipeline);

    WGPUBindGroupLayout layout
      = wgpuRenderPipelineGetBindGroupLayout(lines2D_pipeline, 0);

    {
        // create uniform buffer
        WGPUBufferDescriptor uniform_buffer_desc = {};
        uniform_buffer_desc.size                 = sizeof(Uniforms);
        uniform_buffer_desc.usage
          = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;

        lines2d_uniform_buffer
          = wgpuDeviceCreateBuffer(gctx->device, &uniform_buffer_desc);
        ASSERT(lines2d_uniform_buffer);

        // create positions storage buffer
        WGPUBufferDescriptor positions_buffer_desc = {};
        positions_buffer_desc.size                 = sizeof(line_points);
        positions_buffer_desc.usage
          = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
        lines2d_positions
          = wgpuDeviceCreateBuffer(gctx->device, &positions_buffer_desc);

        // create bind group entry for uniforms
        WGPUBindGroupEntry entries[2]     = {};
        WGPUBindGroupEntry* uniform_entry = &entries[0];
        uniform_entry->binding            = 0;
        uniform_entry->buffer             = lines2d_uniform_buffer;
        uniform_entry->offset             = 0;
        uniform_entry->size               = sizeof(Uniforms);

        // create bind group entry for positions
        WGPUBindGroupEntry* positions_entry = &entries[1];
        positions_entry->binding            = 1;
        positions_entry->buffer             = lines2d_positions;
        positions_entry->offset             = 0;
        positions_entry->size               = positions_buffer_desc.size;

        // create bind group
        WGPUBindGroupDescriptor bg_desc = {};
        bg_desc.layout                  = layout;
        bg_desc.entryCount              = ARRAY_LENGTH(entries);
        bg_desc.entries                 = entries;
        ASSERT(bg_desc.entryCount == 2);

        lines2d_bind_group = wgpuDeviceCreateBindGroup(ctx->device, &bg_desc);
    }
}

// update all uniform and storage buffers
// Note: in actual app, would only update storage buffers (vertices, indices)
// if they actually changed
static void updateBuffers(glm::mat4 proj, glm::mat4 view, glm::vec3 camPos)
{

    //   struct Uniforms {
    //     glm::mat4x4 world;   // at byte offset 0
    //     glm::mat4x4 view;    // at byte offset 64
    //     glm::mat4x4 proj;    // at byte offset 128
    //     float screen_width;  // at byte offset 192
    //     float screen_height; // at byte offset 196
    //     float _pad0[2];
    // };

    UNUSED_VAR(camPos);

    // update uniform buffer
    int screen_width, screen_height;
    glfwGetWindowSize(window, &screen_width, &screen_height);
    Uniforms uniforms         = {};
    uniforms.world            = glm::mat4(1.0f);
    uniforms.view             = view;
    uniforms.proj             = proj;
    uniforms.screen_width     = (float)screen_width;
    uniforms.screen_height    = (float)screen_height;
    uniforms.line_point_count = num_points;
    uniforms.line_width       = line_width;
    wgpuQueueWriteBuffer(gctx->queue, lines2d_uniform_buffer, 0, &uniforms,
                         sizeof(Uniforms));

    { // update positions buffer

        // create sentinal start point
        if (line_loop) {
            line_points[0] = line_points[num_points]; // copy last point
        } else {
            glm::vec3 diff = line_points[2] - line_points[1];
            line_points[0]
              = line_points[1] - diff; // extend in opposite direction
        }

        // sentinal end points
        if (line_loop) {
            line_points[num_points + 1] = line_points[1]; // copy first point
            line_points[num_points + 2] = line_points[2]; // copy second point
        } else {
            glm::vec3 diff
              = line_points[num_points] - line_points[num_points - 1];
            line_points[num_points + 1]
              = line_points[num_points] + diff; // extend in same direction
        }

        wgpuQueueWriteBuffer(gctx->queue, lines2d_positions, 0, line_points,
                             sizeof(line_points));
    }
}

static void _Test_Lines2D_onUpdate(f32 dt)
{
}

static void drawUI(WGPURenderPassEncoder pass)
{
    using namespace ImGui;
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // enable docking to main window
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    // ImGui::ShowDemoWindow(NULL);

    // docking:
    // https://gist.github.com/AidanSun05/b342f1bf023e931695473e343569759c

    ImGui::Begin("Lines2D");

    DragFloat("Line Width", &line_width, 0.001f, 0.0f, 1.0f);

    Checkbox("Loop", &line_loop);

    // modify indices 1 --> num_points-2. Endpoints (0 and num_points-1) are
    // reserved for line loop / sentinel endpoints
    for (size_t i = 0; i < num_points; ++i) {
        int idx = i + 1;
        PushID(idx);
        DragFloat3("##point", (float*)&line_points[idx], 0.01f);
        SameLine();
        if (Button("Remove")) {
            for (size_t j = idx; j < num_points - 1; ++j) {
                line_points[j] = line_points[j + 1];
            }
            num_points = MAX(num_points - 1, 0);
        }
        PopID();
    }
    if (Button("Add Point")) {
        num_points = MIN(num_points + 1, MAX_LINE_POINTS);
    }

    // listbox display all line points, including sentinals
    for (size_t i = 0; i < num_points + 2; ++i) {
        Text("Point %zu: (%.2f, %.2f)", i, line_points[i].x, line_points[i].y);
    }

    ImGui::End();

    // NEXT: add UI for adding/removing points, using multi-component drag
    // sliders

    ImGui::Render();
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
}

static void _Test_Lines2D_onRender(glm::mat4 proj, glm::mat4 view,
                                   glm::vec3 camPos)
{
    updateBuffers(proj, view, camPos);

    GraphicsContext::prepareFrame(gctx);

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(
      gctx->commandEncoder, &gctx->renderPassDesc);

    wgpuRenderPassEncoderSetPipeline(renderPass, lines2D_pipeline);

    wgpuRenderPassEncoderSetBindGroup(renderPass, 0, lines2d_bind_group, 0, 0);

    // wgpuRenderPassEncoderDraw(
    // WGPURenderPassEncoder renderPassEncoder,
    // uint32_t vertexCount,
    // uint32_t instanceCount,
    // uint32_t firstVertex,
    // uint32_t firstInstance);

    const u32 vertex_count = line_loop ? 2 * (num_points + 1) : 2 * num_points;
    wgpuRenderPassEncoderDraw(renderPass, vertex_count, 1, 0, 0);
    // wgpuRenderPassEncoderDraw(
    //   renderPass, vertex_count, 1,
    //   2, // first vertex is 2 to skip two vertices of sentinel start point
    //   0);

    drawUI(renderPass);

    wgpuRenderPassEncoderEnd(renderPass);

    GraphicsContext::presentFrame(gctx);
}

static void _Test_Lines2D_onExit()
{
}

void Test_Lines2D(TestCallbacks* callbacks)
{
    *callbacks          = {};
    callbacks->onInit   = _Test_Lines2D_onInit;
    callbacks->onUpdate = _Test_Lines2D_onUpdate;
    callbacks->onRender = _Test_Lines2D_onRender;
    callbacks->onExit   = _Test_Lines2D_onExit;
}

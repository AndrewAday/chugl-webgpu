#include "test_base.h"

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_wgpu.h>
#include <imgui/imgui.h>

#include <webgpu/webgpu.h>

static GraphicsContext* gctx = NULL;
static GLFWwindow* window    = NULL;

static void _Test_ImGUI_onInit(GraphicsContext* ctx, GLFWwindow* w)
{
    gctx   = ctx;
    window = w;
}

static bool show_demo_window    = true;
static bool show_another_window = true;
static ImVec4 clear_color       = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

static void _Test_ImGUI_onRender(glm::mat4 proj, glm::mat4 view,
                                 glm::vec3 camPos)
{
    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    // 1. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    {
        static float f     = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!"
                                       // and append into it.

        ImGui::Text("This is some useful text."); // Display some text (you can
                                                  // use a format strings too)
        ImGui::Checkbox(
          "Demo Window",
          &show_demo_window); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat(
          "float", &f, 0.0f,
          1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3(
          "clear color",
          (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button(
              "Button")) // Buttons return true when clicked (most widgets
                         // return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // Rendering (prepares ImDrawList)
    ImGui::Render();

    // pass encoder
    WGPUCommandEncoderDescriptor enc_desc = {};
    WGPUCommandEncoder encoder
      = wgpuDeviceCreateCommandEncoder(gctx->device, &enc_desc);

    // start render pass (depends on color attachment and pass descriptor)
    // Need a different render pass encoder for different color attachment views
    // and depth stencil views
    WGPURenderPassColorAttachment color_attachments = {};
    color_attachments.loadOp                        = WGPULoadOp_Clear;
    color_attachments.storeOp                       = WGPUStoreOp_Store;
    color_attachments.clearValue
      = { clear_color.x * clear_color.w, clear_color.y * clear_color.w,
          clear_color.z * clear_color.w, clear_color.w };
    color_attachments.view
      = wgpuSwapChainGetCurrentTextureView(gctx->swapChain);

    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.colorAttachmentCount     = 1;
    render_pass_desc.colorAttachments         = &color_attachments;
    render_pass_desc.depthStencilAttachment   = nullptr;

    WGPURenderPassEncoder pass
      = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);

    // actual draw call
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);

    // multiple viewports (not working)
    // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //     GLFWwindow* backup_current_context = glfwGetCurrentContext();
    //     ImGui::UpdatePlatformWindows();
    //     ImGui::RenderPlatformWindowsDefault();
    //     glfwMakeContextCurrent(backup_current_context);
    // }

    // end render pass
    wgpuRenderPassEncoderEnd(pass);

    // submit command buffer
    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    WGPUCommandBuffer cmd_buffer
      = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    WGPUQueue queue = wgpuDeviceGetQueue(gctx->device);
    wgpuQueueSubmit(queue, 1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
    wgpuSwapChainPresent(gctx->swapChain);
#endif

    wgpuTextureViewRelease(color_attachments.view);
    wgpuRenderPassEncoderRelease(pass);
    wgpuCommandEncoderRelease(encoder);
    wgpuCommandBufferRelease(cmd_buffer);
}

static void _Test_ImGUI_onExit()
{
    // Cleanup
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Test_ImGUI(TestCallbacks* callbacks)
{
    *callbacks          = {};
    callbacks->onInit   = _Test_ImGUI_onInit;
    callbacks->onRender = _Test_ImGUI_onRender;
    callbacks->onExit   = _Test_ImGUI_onExit;
}
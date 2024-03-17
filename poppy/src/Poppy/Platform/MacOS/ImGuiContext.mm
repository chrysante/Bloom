#include "Poppy/UI/ImGuiContext.h"

#include <AppKit/AppKit.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_osx.h>
#include <backends/imgui_impl_metal.h>

#include "Bloom/Application.h"
#include "Bloom/GPU.h"
#include "Bloom/Platform/Metal/MetalDevice.h"
#include "Bloom/Platform/Metal/MetalCommandQueue.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"
#include "Poppy/Core/Debug.h"

using namespace bloom;
using namespace mtl::short_types;

using namespace poppy;

static NSView* getNativeView(bloom::Window& window) {
    NSWindow* nsWindow = (__bridge NSWindow*)window.nativeHandle();
    return nsWindow.contentView;
}

void poppy::ImGuiContext::doInitPlatform(bloom::HardwareDevice& device,
                                         bloom::Window& window) {
    ImGui_ImplMetal_Init(dynamic_cast<MetalDevice&>(device).device);
    ImGui_ImplOSX_Init(getNativeView(window));
}

void poppy::ImGuiContext::doNewFramePlatform(bloom::Window& window) {
    ImGui_ImplOSX_NewFrame(getNativeView(window));
}

void poppy::ImGuiContext::doDrawFramePlatform(bloom::HardwareDevice&, bloom::Window& window) {
    MetalCommandQueue& mtlCommandQueue = dynamic_cast<MetalCommandQueue&>(window.commandQueue());
    id<MTLCommandBuffer> commandBuffer = [mtlCommandQueue.queue commandBuffer];
    MTLRenderPassDescriptor* renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
    MTLRenderPassColorAttachmentDescriptor* caDesc = [[MTLRenderPassColorAttachmentDescriptor alloc] init];
    std::unique_ptr const backbuffer = window.swapchain().nextBackbuffer();
    caDesc.texture = (__bridge id<MTLTexture>)backbuffer->texture().nativeHandle();
    caDesc.loadAction = MTLLoadActionClear;
    caDesc.storeAction = MTLStoreActionStore;
    [renderPassDescriptor.colorAttachments setObject: caDesc
                                  atIndexedSubscript: 0];
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    // Rendering
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
    id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
    // Present
    auto& mtlBackbuffer = dynamic_cast<MetalBackbuffer&>(*backbuffer);
    [commandBuffer presentDrawable: mtlBackbuffer.drawable];
    [commandBuffer commit];
    Logger::Trace("Committed render commands");
}

void poppy::ImGuiContext::doShutdownPlatform() {
    ImGui_ImplOSX_Shutdown();
    ImGui_ImplMetal_Shutdown();
}

void poppy::ImGuiContext::createFontAtlasPlatform(ImFontAtlas* atlas, bloom::HardwareDevice& device) {
    MetalDevice& mtlDevice = dynamic_cast<MetalDevice&>(device);
    ImGui_ImplMetal_CreateFontsTexture(mtlDevice.device);
}

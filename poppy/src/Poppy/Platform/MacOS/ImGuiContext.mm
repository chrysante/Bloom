#include "Poppy/UI/ImGuiContext.hpp"

#import <AppKit/AppKit.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "Poppy/Platform/MacOS/imgui_impl_osx.h"
#include "Poppy/Platform/MacOS/imgui_impl_metal.h"
#include "Bloom/Application.hpp"
#include "Bloom/GPU.hpp"
#include "Bloom/Platform/Metal/MetalDevice.h"
#include "Bloom/Platform/Metal/MetalCommandQueue.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"
#include "Poppy/Core/Debug.hpp"

using namespace bloom;
using namespace mtl::short_types;

using namespace poppy;

void poppy::ImGuiContext::doInitPlatform(bloom::HardwareDevice& device) {
    // Setup Renderer backend
    MetalDevice& mtlDevice = utl::down_cast<MetalDevice&>(device);
    ImGui_ImplMetal_Init(mtlDevice.device);
    ImGui_ImplOSX_Init();
}

void poppy::ImGuiContext::doNewFramePlatform(bloom::Window& window) {
    NSWindow* nsWindow = (__bridge NSWindow*)window.nativeHandle();
    ImGui_ImplOSX_NewFrame(nsWindow.contentView);
}

void poppy::ImGuiContext::doDrawFramePlatform(bloom::HardwareDevice&, bloom::Window& window) {
    MetalCommandQueue& mtlCommandQueue = utl::down_cast<MetalCommandQueue&>(window.commandQueue());
    
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

    auto& mtlBackbuffer = utl::down_cast<MetalBackbuffer&>(*backbuffer);
    
    // Present
    [commandBuffer presentDrawable: mtlBackbuffer.drawable];
    [commandBuffer commit];
}

void poppy::ImGuiContext::doShutdownPlatform() {
    
}

void poppy::ImGuiContext::createFontAtlasPlatform(ImFontAtlas* atlas, bloom::HardwareDevice& device) {
    MetalDevice& mtlDevice = utl::down_cast<MetalDevice&>(device);
    ImGui_ImplMetal_CreateFontsTexture(mtlDevice.device);
}

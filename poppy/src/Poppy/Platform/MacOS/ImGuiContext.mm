#include "Poppy/UI/ImGuiContext.h"

#include <AppKit/AppKit.h>
#include <backends/imgui_impl_metal.h>
#include <backends/imgui_impl_osx.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "Bloom/Application.h"
#include "Bloom/GPU.h"
#include "Bloom/Platform/Metal/MetalCommandQueue.h"
#include "Bloom/Platform/Metal/MetalDevice.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

static NSView* getContentView(Window& window) {
    NSWindow* nsWindow = (__bridge NSWindow*)window.nativeHandle();
    return nsWindow.contentView;
}

void poppy::ImGuiContext::doInitPlatform(HardwareDevice& device,
                                         Window& window) {
    ImGui_ImplMetal_Init(dynamic_cast<MetalDevice&>(device).device);
    ImGui_ImplOSX_Init(getContentView(window));
}

void poppy::ImGuiContext::doNewFramePlatform(Window& window) {
    ImGui_ImplOSX_NewFrame(getContentView(window));
}

void poppy::ImGuiContext::doDrawFramePlatform(HardwareDevice&, Window& window) {
    MetalCommandQueue& mtlCommandQueue =
        dynamic_cast<MetalCommandQueue&>(window.commandQueue());
    id<MTLCommandBuffer> commandBuffer = [mtlCommandQueue.queue commandBuffer];
    MTLRenderPassDescriptor* renderPassDescriptor =
        [[MTLRenderPassDescriptor alloc] init];
    MTLRenderPassColorAttachmentDescriptor* caDesc =
        [[MTLRenderPassColorAttachmentDescriptor alloc] init];
    std::unique_ptr const backbuffer = window.swapchain().nextBackbuffer();
    caDesc.texture =
        (__bridge id<MTLTexture>)backbuffer->texture().nativeHandle();
    caDesc.loadAction = MTLLoadActionClear;
    caDesc.storeAction = MTLStoreActionStore;
    [renderPassDescriptor.colorAttachments setObject:caDesc
                                  atIndexedSubscript:0];
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    // Rendering
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    renderPassDescriptor.colorAttachments[0].clearColor =
        MTLClearColorMake(0, 0, 0, 1);
    id<MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
    // Present
    auto& mtlBackbuffer = dynamic_cast<MetalBackbuffer&>(*backbuffer);
    [commandBuffer presentDrawable:mtlBackbuffer.drawable];
    [commandBuffer commit];
}

void poppy::ImGuiContext::doShutdownPlatform() {
    ImGui_ImplOSX_Shutdown();
    ImGui_ImplMetal_Shutdown();
}

void poppy::ImGuiContext::uploadCurrentFontAtlas(HardwareDevice& device) {
    MetalDevice& mtlDevice = dynamic_cast<MetalDevice&>(device);
    ImGui_ImplMetal_CreateFontsTexture(mtlDevice.device);
}

/// TODO: Move this to other file
void poppy::postEmptySystemEvent() {
    NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                        location:NSMakePoint(0, 0)
                                   modifierFlags:0
                                       timestamp:0
                                    windowNumber:0
                                         context:nil
                                         subtype:0
                                           data1:0
                                           data2:0];
    [NSApp postEvent:event atStart:YES];
}

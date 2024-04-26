#include "Poppy/UI/ImGuiContext.h"

#include <AppKit/AppKit.h>
#include <Poppy/Platform/MacOS/imgui_impl_metal.h>
#include <Poppy/Platform/MacOS/imgui_impl_osx.h>
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
using namespace vml::short_types;
using namespace poppy;

void poppy::ImGuiContext::initPlatform() {
    ImGui_ImplMetal_Init(*mEditor);
    ImGui_ImplOSX_Init(*mEditor);
}

void poppy::ImGuiContext::newFramePlatform() {
    ImGui_ImplOSX_NewFrame(*mEditor);
}

static void drawMainWindow(Window& window) {
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

static void drawSecondaryWindow(ImGuiViewportP* viewport) {
    BL_UNIMPLEMENTED();
#if 0
    ImGuiViewportDataMetal* data = (ImGuiViewportDataMetal*)viewport->RendererUserData;
    void* handle = viewport->PlatformHandleRaw;
    NSWindow* window = (__bridge NSWindow*)handle;
    // Always render the first frame, regardless of occlusionState, to avoid an initial flicker
    if ((window.occlusionState & NSWindowOcclusionStateVisible) == 0 && !data->FirstFrame)
    {
        // Do not render windows which are completely occluded. Calling -[CAMetalLayer nextDrawable] will hang for
        // approximately 1 second if the Metal layer is completely occluded.
        return;
    }
    data->FirstFrame = false;

    viewport->DpiScale = (float)window.backingScaleFactor;
    if (data->MetalLayer.contentsScale != viewport->DpiScale)
    {
        data->MetalLayer.contentsScale = viewport->DpiScale;
        data->MetalLayer.drawableSize = MakeScaledSize(window.frame.size, viewport->DpiScale);
    }
    viewport->DrawData->FramebufferScale = ImVec2(viewport->DpiScale, viewport->DpiScale);
    id <CAMetalDrawable> drawable = [data->MetalLayer nextDrawable];
    if (drawable == nil)
        return;
    MTLRenderPassDescriptor* renderPassDescriptor = data->RenderPassDescriptor;
    renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    if ((viewport->Flags & ImGuiViewportFlags_NoRendererClear) == 0)
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

    id <MTLCommandBuffer> commandBuffer = [data->CommandQueue commandBuffer];
    id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    ImGui_ImplMetal_RenderDrawData(viewport->DrawData, commandBuffer, renderEncoder);
    [renderEncoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
#endif
}

void poppy::ImGuiContext::drawFramePlatform() {
    BL_PROFILE_FUNCTION();
    if (!mEditor->mainWindow()) {
        return;
    }
    drawMainWindow(*mEditor->mainWindow());
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++) {
        ImGuiViewport* viewport = platform_io.Viewports[i];
        if (viewport->Flags & ImGuiViewportFlags_IsMinimized) continue;
        // drawSecondaryWindow((ImGuiViewportP*)viewport);
        ImGui_ImplMetal_RenderWindow(viewport, nullptr);
    }
}

void poppy::ImGuiContext::shutdownPlatform() {
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

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

static void assignBackbufferToRPDesc(MTLRenderPassDescriptor* desc,
                                     Backbuffer& backbuffer) {
    MTLRenderPassColorAttachmentDescriptor* caDesc =
        [[MTLRenderPassColorAttachmentDescriptor alloc] init];
    caDesc.texture =
        (__bridge id<MTLTexture>)backbuffer.texture().nativeHandle();
    caDesc.loadAction = MTLLoadActionClear;
    caDesc.storeAction = MTLStoreActionStore;
    [desc.colorAttachments setObject:caDesc atIndexedSubscript:0];
    desc.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
}

struct poppy::ImGuiContext::BackendImpl {
    static bool drawWindow(poppy::ImGuiContext& ctx, Window& window,
                           ImGuiViewport* viewport,
                           MTLRenderPassDescriptor* renderPassDescriptor,
                           int index) {
        if (index == 0) {
            ImGui_ImplMetal_NewFrame(window.swapchain()->description());
        }

        // Render
        std::unique_lock lock(ctx.drawMutex);
        if (!ctx.drawDataValid) {
            return false;
        }
        ImDrawData* drawData = viewport->DrawData;
        auto backbuffer = window.swapchain()->nextBackbuffer();
        // TODO: Check if we got a valid backbuffer
        assignBackbufferToRPDesc(renderPassDescriptor, *backbuffer);
        MetalCommandQueue& mtlCommandQueue =
            dynamic_cast<MetalCommandQueue&>(*window.commandQueue());
        id<MTLCommandBuffer> commandBuffer =
            [mtlCommandQueue.queue commandBuffer];
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer
            renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
        ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
        [renderEncoder popDebugGroup];
        [renderEncoder endEncoding];
        lock.unlock();

        // Present
        auto& mtlBackbuffer = dynamic_cast<MetalBackbuffer&>(*backbuffer);
        [commandBuffer presentDrawable:mtlBackbuffer.drawable];
        [commandBuffer commit];
        return true;
    }
};

void poppy::ImGuiContext::drawFramePlatform() {
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    MTLRenderPassDescriptor* renderPassDescriptor =
        [[MTLRenderPassDescriptor alloc] init];
    for (int i = 0; i < platform_io.Viewports.Size; i++) {
        auto* viewport = (ImGuiViewportP*)platform_io.Viewports[i];
        if (viewport->Flags & ImGuiViewportFlags_IsMinimized) {
            continue;
        }
        if (!BackendImpl::drawWindow(*this, *(Window*)viewport->PlatformHandle,
                                     viewport, renderPassDescriptor, i))
        {
            break;
        }
    }
    didRender = true;
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

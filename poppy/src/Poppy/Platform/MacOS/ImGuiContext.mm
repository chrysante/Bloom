#include "Poppy/UI/ImGuiContext.h"

#include <AppKit/AppKit.h>
#include <Poppy/Platform/MacOS/imgui_impl_metal.h>
#include <Poppy/Platform/MacOS/imgui_impl_osx.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "Bloom/Application.h"
#include "Bloom/GPU.h"
#include "Bloom/Platform/Metal/MetalObjects.h"
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
    @autoreleasepool {
        ImGui_ImplOSX_NewFrame(*mEditor);
        ImGui_ImplMetal_NewFrame(
            mEditor->mainWindow()->swapchain()->description());
    }
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
    static void drawWindow(poppy::ImGuiContext& ctx, ImDrawData* drawData,
                           MetalSwapchain& swapchain,
                           MetalCommandQueue& commandQueue,
                           MTLRenderPassDescriptor* renderPassDescriptor) {
        // Render
        std::unique_lock lock(ctx.drawMutex);
        if (!ctx.drawDataValid) {
            return;
        }
        auto backbuffer = swapchain.nextBackbuffer();
        // TODO: Check if we got a valid backbuffer
        assignBackbufferToRPDesc(renderPassDescriptor, *backbuffer);
        id<MTLCommandBuffer> commandBuffer = [commandQueue.queue commandBuffer];
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
    }
};

void poppy::ImGuiContext::drawFramePlatform() {
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    MTLRenderPassDescriptor* renderPassDescriptor =
        [[MTLRenderPassDescriptor alloc] init];
    for (int i = 0; i < platform_io.Viewports.Size; i++) {
        auto* viewport = (ImGuiViewportP*)platform_io.Viewports[i];
        if (!viewport || viewport->Flags & ImGuiViewportFlags_IsMinimized) {
            continue;
        }
        auto& window = *(Window*)viewport->PlatformHandle;
        NSWindow* native = (__bridge NSWindow*)window.nativeHandle();
        if ((native.occlusionState & NSWindowOcclusionStateVisible) == 0) {
            continue;
        }
        ++numRenderTasks;
        mEditor->onRenderThread([=, this, swapchain = window.swapchain(),
                                 commandQueue = window.commandQueue()] {
            @autoreleasepool {
                BackendImpl::drawWindow(*this, viewport->DrawData,
                                        dynamic_cast<MetalSwapchain&>(
                                            *swapchain),
                                        dynamic_cast<MetalCommandQueue&>(
                                            *commandQueue),
                                        renderPassDescriptor);
            }
            --numRenderTasks;
        });
    }
}

void poppy::ImGuiContext::shutdownPlatform() {
    @autoreleasepool {
        ImGui_ImplOSX_Shutdown();
        ImGui_ImplMetal_Shutdown();
    }
}

void poppy::ImGuiContext::uploadCurrentFontAtlas(HardwareDevice& device) {
    @autoreleasepool {
        MetalDevice& mtlDevice = dynamic_cast<MetalDevice&>(device);
        ImGui_ImplMetal_CreateFontsTexture(mtlDevice.device);
    }
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

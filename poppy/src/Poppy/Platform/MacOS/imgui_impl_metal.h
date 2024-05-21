#ifndef POPPY_PLATFORM_MACOS_IMGUI_IMPL_METAL_H
#define POPPY_PLATFORM_MACOS_IMGUI_IMPL_METAL_H

// clang-format off

// dear imgui: Renderer Backend for Metal
// This needs to be used along with a Platform Backend (e.g. OSX)

// Implemented features:
//  [X] Renderer: User texture binding. Use 'MTLTexture' as ImTextureID. Read the FAQ about ImTextureID!
//  [X] Renderer: Large meshes support (64k+ vertices) with 16-bit indices.
//  [X] Renderer: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

// You can use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// Prefer including the entire imgui/ repository into your project (either as a copy or as a submodule), and only build the backends you need.
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include <imgui.h>      // IMGUI_IMPL_API
#include "Bloom/GPU/Swapchain.h"
#ifndef IMGUI_DISABLE


namespace poppy {

class Editor;

} // namespace poppy

//-----------------------------------------------------------------------------
// ObjC API
//-----------------------------------------------------------------------------

#ifdef __OBJC__

@class MTLRenderPassDescriptor;
@protocol MTLDevice, MTLCommandBuffer, MTLRenderCommandEncoder;

IMGUI_IMPL_API bool ImGui_ImplMetal_Init(poppy::Editor& editor);
IMGUI_IMPL_API void ImGui_ImplMetal_Shutdown();
IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(MTLRenderPassDescriptor* renderPassDescriptor);
IMGUI_IMPL_API void ImGui_ImplMetal_NewFrame(bloom::SwapchainDescription const& desc);
IMGUI_IMPL_API void ImGui_ImplMetal_RenderDrawData(ImDrawData* drawData,
                                                   id<MTLCommandBuffer> commandBuffer,
                                                   id<MTLRenderCommandEncoder> commandEncoder,
                                                   vml::usize2 framebufferSize);
IMGUI_IMPL_API void ImGui_ImplMetal_RenderWindow(ImGuiViewport* viewport, void*);

// Called by Init/NewFrame/Shutdown
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateFontsTexture(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyFontsTexture();
IMGUI_IMPL_API bool ImGui_ImplMetal_CreateDeviceObjects(id<MTLDevice> device);
IMGUI_IMPL_API void ImGui_ImplMetal_DestroyDeviceObjects();

#endif

//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE

#endif // POPPY_PLATFORM_MACOS_IMGUI_IMPL_METAL_H

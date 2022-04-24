#include "ImGui.hpp"

#include "Bloom/Core/Debug.hpp"
#include "Bloom/Application/Resource.hpp"
#import "Bloom/Platform/macOS/AppViewController.h"
#import "Bloom/Platform/Metal/MetalRenderContext.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_metal.h>
#include <imgui/backends/imgui_impl_osx.h>

#include <imgui/ImGuizmo.h>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <utl/utility.hpp>

namespace poppy {
	
	std::string_view toString(FontWeight w) {
		return std::array{
			"Ultralight",
			"Thin",
			"Light",
			"Regular",
			"Medium",
			"Semibold",
			"Bold",
			"Heavy",
			"Black"
		}[utl::to_underlying(w)];
	}
	
	std::string_view toString(FontStyle s) {
		return std::array{
			"Roman",
			"Italic"
		}[utl::to_underlying(s)];
	}
	
	ImGuiContext* ImGuiContext::s_instance = nullptr;
	
	ImGuiContext::~ImGuiContext() {
		shutdown();
	}
	
	void ImGuiContext::init(bloom::RenderContext* rc) {
		assert(s_instance == nullptr && "Called init before?");
		s_instance = this;
		renderContext = rc;
		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderContext);
		AppViewController* const viewController = (__bridge AppViewController*)mtlRenderContext->viewController();
		id<MTLDevice> const device = (__bridge id<MTLDevice>)mtlRenderContext->device();
		
		IMGUI_CHECKVERSION();
		_context = ImGui::CreateContext();

		// Set .ini filename
		_iniFilename = (bloom::getLibraryDir() / "Poppy/imgui.ini").string();
		_context->IO.IniFilename = _iniFilename.data();
		
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
		io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
		
		// Setup Dear ImGui style
	    ImGui::StyleColorsDark();

		// Setup Renderer backend
		ImGui_ImplMetal_Init(device);

		// load fonts
		for (auto w: utl::enumerate<FontWeight>()) {
			for (auto s: utl::enumerate<FontStyle>()) {
				fontMap.insert({ { w, s }, loadFont(w, s) });
			}
		}
		this->_defaultFont = getFont(FontWeight::regular, FontStyle::roman);
		
		ImGui_ImplOSX_Init(viewController.view);
		
		auto presentKeyEventHandler = viewController.keyEventResponder.keyEventHandler;
		auto keyEventHandler = [=](NSEvent* _Nonnull event) -> bool {
			if (ImGui::GetIO().WantCaptureKeyboard) {
				return ImGui_ImplOSX_HandleEvent(event, nil);
			}
			
			return presentKeyEventHandler ? presentKeyEventHandler(event) : false;			
		};
		viewController.keyEventResponder.keyEventHandler = keyEventHandler;
		
		auto insertTextHandler = [](id _Nonnull aString, NSRange) {
			ImGuiIO& io = ImGui::GetIO();

			NSString* characters;
			if ([aString isKindOfClass:[NSAttributedString class]])
				characters = [aString string];
			else
				characters = (NSString*)aString;

			io.AddInputCharactersUTF8(characters.UTF8String);
		};
		
		viewController.keyEventResponder.insertTextHandler = insertTextHandler;
	}
	
	void ImGuiContext::shutdown() {
		if (_context == nullptr) {
			return;
		}
		ImGui::DestroyContext(_context);
		_context = nullptr;
	}
	
	void ImGuiContext::beginFrame() {
		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderContext);
		AppViewController* const viewController = (__bridge AppViewController*)mtlRenderContext->viewController();
		MTKView* const view = viewController.mtkView;
		
		ImGuiIO& io = ImGui::GetIO();
		
		io.DisplaySize.x = view.bounds.size.width;
		io.DisplaySize.y = view.bounds.size.height;

		CGFloat framebufferScale = view.window.screen.backingScaleFactor ?: NSScreen.mainScreen.backingScaleFactor;
		io.DisplayFramebufferScale = ImVec2(framebufferScale, framebufferScale);

		io.DeltaTime = 1 / float(view.preferredFramesPerSecond ?: 60);

		id<MTLCommandBuffer> commandBuffer = [viewController.commandQueue commandBuffer];

		MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;
		if (renderPassDescriptor == nil)
		{
			[commandBuffer commit];
			return;
		}

		// Start the Dear ImGui frame
		ImGui_ImplMetal_NewFrame(renderPassDescriptor);
		ImGui_ImplOSX_NewFrame(view);
		ImGui::NewFrame();
		
		ImGui::PushFont((ImFont*)_defaultFont);
	
		_renderPassDescriptor = (void*)CFBridgingRetain(renderPassDescriptor);
		_commandBuffer = (void*)CFBridgingRetain(commandBuffer);
	}
	
	void ImGuiContext::present() {
		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderContext);
		AppViewController* const viewController = (__bridge AppViewController*)mtlRenderContext->viewController();
		
		MTKView* const view = viewController.mtkView;
		
		MTLRenderPassDescriptor* const renderPassDescriptor = (MTLRenderPassDescriptor*)CFBridgingRelease(_renderPassDescriptor);
		id<MTLCommandBuffer> commandBuffer = (id<MTLCommandBuffer>)CFBridgingRelease(_commandBuffer);
		
		ImGui::PopFont();
		
		// Rendering
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		
		renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
		id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
		[renderEncoder pushDebugGroup:@"Dear ImGui rendering"];
		ImGui_ImplMetal_RenderDrawData(draw_data, commandBuffer, renderEncoder);
		[renderEncoder popDebugGroup];
		[renderEncoder endEncoding];

		// Present
		[commandBuffer presentDrawable: view.currentDrawable];
		[commandBuffer commit];
	}
	
	void ImGuiContext::handleMouseEvent(void* nativeEvent) {
		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderContext);
		AppViewController* const viewController = (__bridge AppViewController*)mtlRenderContext->viewController();
		
		NSEvent* event = (__bridge NSEvent*)nativeEvent;
		ImGui_ImplOSX_HandleEvent(event, viewController.view);
	}
	
	void* ImGuiContext::getFont(FontWeight weight, FontStyle style) {
		auto const key = std::pair{ weight, style };
		auto itr = fontMap.find(key);
		assert(itr != fontMap.end());
		return itr->second;
	}
	
	void* ImGuiContext::loadFont(FontWeight weight, FontStyle style) {
		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Read 'docs/FONTS.txt' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		//io.Fonts->AddFontDefault();
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
		//IM_ASSERT(font != NULL);
		
		ImGuiIO& io = ImGui::GetIO();
		
		float const fontSize = 16;
		float const dpiScale = 2;
		
//		char16_t const glyphs[] =
//			u" !\"#$%&'()*+,-./0123456789:;<=>?"
//			u"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
//			u"`abcdefghijklmnopqrstuvwxyz{|}~"
//			u"äöüÄÖÜ" // and so on
//			u"ᴀʙᴄᴅᴇꜰɢʜɪᴊᴋʟᴍɴᴏᴘꞯʀꜱᴛᴜᴠᴡʏᴢ"
		;
		
		auto fontPath = utl::format("Font/SFPro-{}-{}.ttf", toString(weight), toString(style));
		auto result = io.Fonts->AddFontFromFileTTF(bloom::pathForResource(fontPath).string().data(),
												   fontSize * dpiScale
//												   , nullptr, (ImWchar const*)glyphs
												   );
		bloomAssert(result);
		
		io.Fonts->Build();
		io.FontGlobalScale = 1.0 / dpiScale;
		
		auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(renderContext);
		id<MTLDevice> const device = (__bridge id<MTLDevice>)mtlRenderContext->device();
		ImGui_ImplMetal_CreateFontsTexture(device);
		return result;
	}
	
}


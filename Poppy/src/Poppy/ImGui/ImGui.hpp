#pragma once

#include "Bloom/Application/Application.hpp"
#include "Bloom/Graphics/RenderContext.hpp"

#include <string>
#include <utl/hashmap.hpp>
#include <utl/scope_guard.hpp>
#include <imgui.h>

struct ImGuiContext;

struct ImFont;

namespace poppy {
	
	enum struct FontWeight {
		ultralight = 0,
		thin,
		light,
		regular,
		medium,
		semibold,
		bold,
		heavy,
		black,
		_count
	};
	
	std::string_view toString(FontWeight);
	
	enum struct FontStyle {
		roman = 0,
		italic,
		_count
	};
	
	std::string_view toString(FontStyle);
	
	class ImGuiContext {
	public:
		static ImGuiContext& instance() { return *s_instance; }
	private:
		static ImGuiContext* s_instance;
		
	public:
		ImGuiContext() = default;
		ImGuiContext(ImGuiContext const&) = delete;
		~ImGuiContext();
		
		void init(bloom::RenderContext*);
		void shutdown();
		
		void beginFrame();
		void present();
		
		void handleMouseEvent(void* nativeEvent);
		bool handleMouseEvent(bloom::Event const&);
		
		void* getFont(FontWeight, FontStyle);
		
	private:
		void* loadFont(FontWeight, FontStyle);
		
	private:
		bloom::RenderContext* renderContext = nullptr;
		::ImGuiContext* _context = nullptr;
		void* _defaultFont = nullptr;
		void* _renderPassDescriptor;
		void* _commandBuffer;
		std::string _iniFilename;
		using FontMap = utl::hashmap<std::pair<FontWeight, FontStyle>, void*, utl::hash<std::pair<FontWeight, FontStyle>>>;
		FontMap fontMap;
	};
	
	struct ViewportInput {
		bool hovered, held, pressed;
	};
	
	ViewportInput detectViewportInput(int mouseButtons);
	
	bool dragFloat3Pretty(float*, char const* labelID, float speed = 1);
	
	auto withFont(FontWeight w, FontStyle s, std::invocable auto&& f) {
		ImGui::PushFont((ImFont*)ImGuiContext::instance().getFont(w, s));
		utl::scope_guard pop = []{
			ImGui::PopFont();
		};
		return f();
	}
	
	
}

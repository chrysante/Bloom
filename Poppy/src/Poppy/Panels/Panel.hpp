#pragma once

#include <string>
#include <array>
#include <concepts>
#include <mtl/mtl.hpp>
#include <utl/memory.hpp>
#include <yaml-cpp/yaml.h>
#include "Bloom/Application/Event.hpp"


namespace bloom { class Application; }

namespace poppy {
	
	class Editor;
	
	struct PanelOptions {
		bool unique = true;
	};
	
	class Panel {
		friend class Editor;
		
	public:
		Panel(std::string_view title, PanelOptions = {});
		virtual ~Panel() = default;
		
		bool focused() const;
		
		bloom::Application& getApplication();
		bloom::Application const& getApplication() const;
		
		Editor& getEditor() { return *app; }
		Editor const& getEditor() const { return *app; }
		
		mtl::float2 windowSpaceToViewSpace(mtl::float2) const;
		mtl::float2 viewSpaceToWindowSpace(mtl::float2) const;
		
		mtl::float2 location() const { return _viewPosition; }
		mtl::float2 size() const { return _viewSize; }
	
		std::string_view uniqueName() const { return title; }
		
		PanelOptions getOptions() const { return options; }
		
		bool matchesName(std::string_view) const;
		void setFocused();
		
		template <std::derived_from<Panel> T>
		static utl::unique_ref<Panel> create(Editor* editor, auto&&... args)
			requires std::constructible_from<T, decltype(args)...>
		{
			utl::unique_ref<Panel> result = utl::make_unique_ref<T>(UTL_FORWARD(args)...);
			result->app = editor;
			return result;
		}
		
		bool shouldClose() const { return !_open; }
		
	private:
		virtual void init() {};
		virtual void shutdown() {};
		virtual void display() = 0;
		virtual void onEvent(bloom::Event&) {}
		
	protected:
		YAML::Node settings;
		mtl::float2 padding = 5;
		
	private:
		void doDisplay();
		
	private:
		PanelOptions options;
		Editor* app = nullptr;
		std::string title;
		mtl::float2 _windowSize, _viewSize, _viewPosition;
		void* _imguiWindow = nullptr;
		bool _open = true;
	};
	
}

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
	
	class Panel {
		friend class Editor;
		
	public:
		Panel(std::string_view title);
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
	
		std::string_view uniqueName() const { return title[0]; }
	
		template <std::derived_from<Panel> T>
		static utl::unique_ref<Panel> create(Editor* editor, auto&&... args)
			requires std::constructible_from<T, decltype(args)...>
		{
			utl::unique_ref<Panel> result = utl::make_unique_ref<T>(UTL_FORWARD(args)...);
			result->app = editor;
			return result;
		}
		
	private:
		virtual void init() {};
		virtual void shutdown() {};
		virtual void display() = 0;
		virtual void onEvent(bloom::Event const&) {}
		
	protected:
		void ignoreEvents(bloom::EventType mask);
		
		YAML::Node settings;
		mtl::float2 padding = 5;
		
	private:
		void doDisplay();
		
	private:
		Editor* app = nullptr;
		std::array<std::string, 2> title;
		mtl::float2 _windowSize, _viewSize, _viewPosition;
		bloom::EventType _ignoreEventMask = bloom::EventType::none;;
		void* _imguiWindow = nullptr;
	};
	
}

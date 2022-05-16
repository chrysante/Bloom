#pragma once

#include <string>
#include <array>
#include <concepts>
#include <mtl/mtl.hpp>
#include <utl/memory.hpp>
#include <utl/scope_guard.hpp>
#include <utl/messenger.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/InputEvent.hpp"

namespace bloom { class Application; }

namespace poppy {
	
	struct PanelOptions {
		bool unique = true;
	};
	
	class Panel {
		friend class Editor;
		
	public:
		Panel(std::string_view title, PanelOptions = {});
		virtual ~Panel() = default;
		
		bool focused() const;
		
		mtl::float2 windowSpaceToViewSpace(mtl::float2) const;
		mtl::float2 viewSpaceToWindowSpace(mtl::float2) const;
		
		mtl::float2 location() const { return _viewPosition; }
		mtl::float2 size() const { return _viewSize; }
	
		std::string_view uniqueName() const { return title; }
		
		PanelOptions getOptions() const { return options; }
		
		bool matchesName(std::string_view) const;
		void setFocused();
		
		template <std::derived_from<Panel> T>
		static utl::unique_ref<Panel> create(auto&&... args)
			requires std::constructible_from<T, decltype(args)...>
		{
			utl::unique_ref<Panel> result = utl::make_unique_ref<T>(UTL_FORWARD(args)...);
			return result;
		}
		
		bool shouldClose() const { return !_open; }
		
	private:
		virtual void init() {};
		virtual void shutdown() {};
		virtual void display() = 0;
		virtual void onInputEvent(bloom::InputEvent&) {}
		
	protected:
		void beginInactive(bool = true) const;
		void endInactive() const;
		auto inactiveWhenSimulating(std::invocable auto&& block) { return utl::as_const(*this).inactiveWhenSimulating(UTL_FORWARD(block)); }
		auto inactiveWhenSimulating(std::invocable auto&& block) const;
		bool isSimulating() const;
		
		void emptyWithReason(std::string_view reason) const;
		
		YAML::Node settings;
		mtl::float2 padding = 5;
		
	private:
		void doDisplay();
		
	private:
		PanelOptions options;
		std::string title;
		mtl::float2 _windowSize, _viewSize, _viewPosition;
		void* _imguiWindow = nullptr;
		bool _open = true;
		utl::listener_id_bag _listenerIDs;
	};

	inline auto Panel::inactiveWhenSimulating(std::invocable auto&& block) const {
		beginInactive(isSimulating());
		utl::scope_guard end = [this]{
			endInactive();
		};
		return block();
	}
	
}

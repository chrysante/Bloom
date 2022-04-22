#include "Panel.hpp"

#include <utl/format.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stdio.hpp>

#include "Editor.hpp"

namespace poppy {
	
	static std::size_t id = 0;
	
	Panel::Panel(std::string_view title):
		title {
			utl::format("{}##{}", title, id),
			utl::format("{}-Child##{}", title, id++)
		}
	{}
	
	bloom::Application& Panel::getApplication() {
		return *app;
	}
	
	bloom::Application const& Panel::getApplication() const {
		return *app;
	}
	
	void Panel::doDisplay() {
		_ignoreEventMask = bloom::EventType::none;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
		if (ImGui::Begin(title[0].data())) {
			ImGui::BeginChild(title[1].data());
			_imguiWindow = GImGui->CurrentWindow;
			_windowSize = ImGui::GetMainViewport()->Size;
			_viewSize = ImGui::GetWindowSize();
			_viewPosition = ImGui::GetWindowPos();
			
			this->display();
			ImGui::EndChild();
			
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}
	
	mtl::float2 Panel::windowSpaceToViewSpace(mtl::float2 position) const {
		return position - _viewPosition;
	}
	
	mtl::float2 Panel::viewSpaceToWindowSpace(mtl::float2 position) const {
		return position + _viewPosition;
	}
	
	void Panel::ignoreEvents(bloom::EventType mask) {
		_ignoreEventMask |= mask;
	}
	
	bool Panel::focused() const {
		return GImGui->NavWindow == _imguiWindow;
	}
	
	
}

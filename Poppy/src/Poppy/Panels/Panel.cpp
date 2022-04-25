#include "Panel.hpp"

#include <utl/format.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stdio.hpp>

#include "Editor.hpp"

namespace poppy {
	
	static std::size_t gID = 0;
	
	Panel::Panel(std::string_view title, PanelOptions options):
		title {
			utl::format("{}##{}", title, gID)
		},
		options(options)
	{ ++gID; }
	
	bloom::Application& Panel::getApplication() {
		return *app;
	}
	
	bloom::Application const& Panel::getApplication() const {
		return *app;
	}
	
	void Panel::doDisplay() {
		_ignoreEventMask = bloom::EventType::none;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
		ImGui::SetNextWindowSize({600, 400});
		
		if (ImGui::Begin(title.data(), &_open)) {
			ImGui::BeginChild("child");
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
	
	bool Panel::matchesName(std::string_view name) const {
		auto const endpos = title.find("##");
		return name == title.substr(0, endpos);
	}
	
	void Panel::setFocused() {
		ImGui::FocusWindow((ImGuiWindow*)_imguiWindow);
	}
	
}

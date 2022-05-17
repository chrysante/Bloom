#include "Panel.hpp"

#include <utl/format.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <utl/stdio.hpp>

#include "Editor.hpp"
#include "Bloom/Scene/SceneSystem.hpp"
 
using namespace bloom;
using namespace mtl::short_types;

namespace poppy {
	
	static std::size_t gID = 0;
	
	Panel::Panel(std::string_view title, PanelOptions options):
		title {
			utl::format("{}##{}", title, gID)
		},
		options(options)
	{ ++gID; }
	
	void Panel::doDisplay() {
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
	
	void Panel::doInit() {
		this->init();
	}
	
	void Panel::doShutdown() {
		this->shutdown();
	}
	
	mtl::float2 Panel::windowSpaceToViewSpace(mtl::float2 position) const {
		return position - _viewPosition;
	}
	
	mtl::float2 Panel::viewSpaceToWindowSpace(mtl::float2 position) const {
		return position + _viewPosition;
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
	
	void Panel::beginInactive(bool b) const {
		ImGui::BeginDisabled(b);
	}
	
	void Panel::endInactive() const {
		ImGui::EndDisabled();
	}
	
	bool Panel::isSimulating() const {
		return Application::get().sceneSystem().isSimulating();
	}
	
	void Panel::emptyWithReason(std::string_view reason) const {
		auto const oldCursorPos = ImGui::GetCursorPos();
		utl::scope_guard reset = [&]{
			ImGui::SetCursorPos(oldCursorPos);
		};
		
		float2 const textSize =	ImGui::CalcTextSize(reason.data());
		
		ImGui::SetCursorPos((size() - textSize) / 2);
		withFont(FontWeight::semibold, FontStyle::roman, [&]{
			ImGui::TextDisabled("%s", reason.data());
		});
		
	}
	
}

#include "DebugViews.hpp"

#include <imgui/imgui.h>
#include "Poppy/UI/Appearance.hpp"

namespace poppy {
	
	void DebugViews::display() {
		if (showDemoWindow) {
			ImGui::ShowDemoWindow(&showDemoWindow);
		}
		if (showUIDebugger) {
			ImGui::ShowMetricsWindow(&showUIDebugger);
		}
		if (showStylePanel) {
			appearance.showInspector(&showStylePanel);
			systemStyleInspector(&showStylePanel);
		}
	}
	
	void DebugViews::menubar() {
		if (ImGui::BeginMenu("Debug")) {
			if (ImGui::MenuItem("Demo Window")) {
				showDemoWindow = true;
			}
			if (ImGui::MenuItem("UI Debugger")) {
				showUIDebugger = true;
			}
			if (ImGui::MenuItem("Style Colors Panel")) {
				showStylePanel = true;
			}
			ImGui::EndMenu();
		}
	}
	
}

#include "Poppy/Editor/Views/DebugViews.h"

#include <imgui.h>

#include "Poppy/UI/Appearance.h"

using namespace poppy;

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
        if (ImGui::MenuItem("Reset style")) {
            ImGui::StyleColorsDark();
        }
        ImGui::EndMenu();
    }
}

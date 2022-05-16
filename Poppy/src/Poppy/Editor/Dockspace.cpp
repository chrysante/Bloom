#include "Dockspace.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Editor.hpp"

#include "Poppy/IconConfig.hpp"

#include "Bloom/Scene/SceneSystem.hpp"

using namespace bloom;

namespace poppy {
    
	void Dockspace::display() {
		dockspace();
		toolbar();
	}
	
	void Dockspace::dockspace() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		auto nextWindowPos = viewport->Pos;
		auto nextWindowSize = viewport->Size;
#if 1
		nextWindowPos.y += toolbarHeight;
		nextWindowSize.y -= toolbarHeight;
#endif
		ImGui::SetNextWindowPos(nextWindowPos);
		ImGui::SetNextWindowSize(nextWindowSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGuiWindowFlags window_flags = 0
			| ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("Master DockSpace", NULL, window_flags);
		ImGuiID dockMain = ImGui::GetID("MyDockspace");

	//	ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		auto const winSizeSaved = style.WindowMinSize.x;
		style.WindowMinSize.x = 250;
		
		int dsFlags = 0;
		dsFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
		dsFlags |= ImGuiDockNodeFlags_NoCloseButton;
		ImGui::DockSpace(dockMain, /* size arg */ {}, dsFlags);
		style.WindowMinSize.x = winSizeSaved;
		ImGui::End();
		ImGui::PopStyleVar(3);
	}
	
	void Dockspace::toolbar() {
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		
		ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x,
									   viewport->Pos.y + ImGui::FindWindowByName("Master DockSpace")->MenuBarHeight()));
		ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, toolbarHeight));
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags window_flags = 0
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoSavedSettings
			;
		
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, GImGui->Style.Colors[ImGuiCol_TitleBg]);
		ImGui::Begin("TOOLBAR", NULL, window_flags);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		auto& sceneSystem = Application::get().sceneSystem();
		bool const isSimulating = sceneSystem.isSimulating();
		
		
		
		ImGui::BeginDisabled(sceneSystem.getScene() == nullptr);
		auto const buttonLabel = !isSimulating ? IconConfig::unicodeStr("play") : IconConfig::unicodeStr("stop");
		ImGui::PushFont((ImFont*)IconConfig::font(16));
		bool const playStop = ImGui::Button(buttonLabel.data(), ImVec2(37, 37));
		ImGui::PopFont();
		if (playStop) {
			auto& editor = Editor::get();
			!isSimulating ? editor.startSimulation() : editor.stopSimulation();
		}
		
		ImGui::EndDisabled();
		
		ImGui::SameLine();
		if (ImGui::Button("Some Other Button", ImVec2(0, 37))) {
			
		}
		ImGui::Separator();
		
		ImGui::End();
	}

}

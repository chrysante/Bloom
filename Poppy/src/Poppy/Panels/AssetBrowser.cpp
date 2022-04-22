#include "AssetBrowser.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "Bloom/Application/Resource.hpp"
#include "Poppy/Editor.hpp"

#include <utl/stdio.hpp>

namespace poppy {
	
	AssetBrowser::AssetBrowser():
		Panel("Asset Browser")
	{
		
	}
	
	void AssetBrowser::init() {
		auto dir = getEditor().settings().getString("Asset Directory");
		if (dir) {
			workingDir = *dir;
		}
	}
	
	static bool firstItemButton = false;
	
	static void beginItemButtons() {
		firstItemButton = true;
	}
	
	static bool itemButton(char const* id, mtl::float2 size) {
		auto const cursorPos = ImGui::GetCursorPos();
		if (!firstItemButton) {
			ImGui::SameLine();
		}
		firstItemButton = false;
		float const availSpace = ImGui::GetContentRegionAvail().x;
		if (availSpace < size.x) {
			ImGui::SetCursorPos(cursorPos);
		}
		return ImGui::Button(id, size);
	}
	
	void AssetBrowser::display() {
		if (workingDir.empty()) {
			if (ImGui::Button("Choose Working Directory...")) {
				bloom::showSelectDirectoryPanel([this](std::string filepath) {
					assert(std::filesystem::exists(filepath));
					workingDir = filepath;
					auto& settings = getEditor().settings();
					settings.setString("Asset Directory", workingDir.string());
				});
			}
			return;
		}
		
		toolbar();
		ImGui::BeginChild("Asset Browser Child");
		
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 15, 15 });
		beginItemButtons();
		for (int i = 0; i < 15; ++i) {
			char id[] = "##item-id-0";
			id[10] += i;
			itemButton(id, itemSize);
		}
		ImGui::PopStyleVar();
		
		ImGui::EndChild();
	}
	
	static bool toolbarButton(char const* label, float height) {
		mtl::float2 size = ImGui::CalcTextSize(label);
		size += 2 * mtl::float2(GImGui->Style.FramePadding);
		size[0] = std::max(size[0], height);
		size[1] = std::max(size[1], height);
		
		return ImGui::Button(label, size);
	}
	
	void AssetBrowser::toolbar() {
		float const toolbarSize = 30;
		
		if (toolbarButton("Up", toolbarSize)) {
			
		}
		ImGui::SameLine();
		if (toolbarButton("Import...", toolbarSize)) {
			
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		ImGui::SliderFloat("Preview Size", &itemSize, 30, 500);
		
		ImGui::Separator();
	}
	
}

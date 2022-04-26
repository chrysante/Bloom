#include "PropertiesPanel.hpp"

#include "Poppy/ImGui/ImGui.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>

using namespace mtl::short_types;

namespace poppy {
	
	
	void PropertiesPanel::header(std::string_view name) {
		withFont(FontWeight::semibold, FontStyle::roman, [&]{
			float4 color = GImGui->Style.Colors[ImGuiCol_Text];
			color.a *= 0.85;
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Text("%s", name.data());
			ImGui::PopStyleColor();
		});
	}
	
	
	bool PropertiesPanel::beginSection() {
		bool const open =  ImGui::BeginTable("Property Table", 2,
											 ImGuiTableFlags_Resizable |
											 ImGuiTableFlags_NoBordersInBodyUntilResize);
		if (open) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f); // Default to 100.0f
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);       // Default to 200.0f
		}
		return open;
	}
	
	void PropertiesPanel::endSection() {
		ImGui::EndTable();
	}
	
	void PropertiesPanel::beginProperty(std::string_view label) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		
		withFont(FontWeight::light, FontStyle::roman, [&]{
			ImGui::SetCursorPosX(3 * GImGui->Style.WindowPadding.x);
			ImGui::Text("%s", label.data());
		});
		
		ImGui::TableSetColumnIndex(1);
	}
	
}

#define IMGUI_DEFINE_MATH_OPERATORS

#include "PropertiesView.hpp"

#include "Poppy/UI/ImGuiHelpers.hpp"
#include "Poppy/Core/Debug.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>

using namespace mtl::short_types;

namespace poppy::propertiesView {
	
	void header(std::string_view name, Font const& font) {
		withFont(font, [&]{
			float4 color = GImGui->Style.Colors[ImGuiCol_Text];
			color.a *= 0.85;
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Text("%s", name.data());
			ImGui::PopStyleColor();
		});
	}
	
	bool beginSection() {
		bool const open =  ImGui::BeginTable("Property Table", 2,
											 ImGuiTableFlags_Resizable |
											 ImGuiTableFlags_NoBordersInBodyUntilResize);
		if (open) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch); // Default to 100.0f
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthStretch);       // Default to 200.0f
		}
		return open;
	}
	
	bool beginSection(std::string_view name, Font const& font) {
		header(name, font);
		return beginSection();
	}
	
	void endSection() {
		ImGui::EndTable();
	}
	
	void beginProperty(std::string_view label) {
		beginProperty(label, Font::UIDefault().setWeight(FontWeight::light));
	}
	
	void beginProperty(std::string_view label, Font const& font) {
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		
		withFont(font, [&]{
			auto& style = ImGui::GetStyle();
			auto const textSize = ImGui::CalcTextSize(label.data());
			auto const availWidth = ImGui::GetContentRegionAvail().x;
			float const cpY = ImGui::GetCursorPosY();
			float const cpX = std::max(style.WindowPadding.x, availWidth - textSize.x);
			
			ImGui::SetCursorPosX(cpX);
			ImGui::Text("%s", label.data());
			if (cpX == style.WindowPadding.x && ImGui::IsItemHovered()) {
				ImGui::SetNextWindowPos(ImGui::GetWindowPos() + ImVec2(cpX, cpY) - style.WindowPadding);
				ImGui::SetTooltip("%s", label.data());
			}
		});
		
		ImGui::TableSetColumnIndex(1);
	}
	
	void fullWidth(float offset) {
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x + offset);
	}
	
}

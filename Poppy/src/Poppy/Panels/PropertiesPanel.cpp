#include "PropertiesPanel.hpp"

#include "Poppy/ImGui/ImGui.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>

using namespace mtl::short_types;

namespace poppy {
	
	
	void PropertiesPanel::header(std::string_view name, FontWeight weight, FontStyle style) {
		withFont(weight, style, [&]{
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
	
	constexpr static std::size_t labelBufferSize = 128;
	std::array<char, labelBufferSize> makeAnonLabel(std::string_view label) {
		bloomAssert(label.size() < labelBufferSize);
		std::array<char, labelBufferSize> result{};
		result[0] = '#';
		result[1] = '#';
		result[2] = '-';
		std::memcpy(result.data() + 3, label.data(), std::min(label.size(), labelBufferSize - 4));
		return result;
	}
	
	bool PropertiesPanel::dragFloat(std::string_view label,
									float* data,
									float speed,
									float min,
									float max,
									char const* format)
	{
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		return ImGui::DragFloat(makeAnonLabel(label).data(), data, speed, min, max, format);
	}
	
	bool PropertiesPanel::sliderFloat(std::string_view label,
									  float* data,
									  float min,
									  float max)
	{
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		return ImGui::SliderFloat(makeAnonLabel(label).data(), data, min, max);
	}
	
	bool PropertiesPanel::sliderInt(std::string_view label,
									int* data,
									int min,
									int max)
	{
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		return ImGui::SliderInt(makeAnonLabel(label).data(), data, min, max);
	}
	
}

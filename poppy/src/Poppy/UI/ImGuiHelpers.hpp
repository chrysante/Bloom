#pragma once

#include "Font.hpp"
#include "Icons.hpp"

#include <array>
#include <imgui/imgui.h>
#include <utl/scope_guard.hpp>

namespace poppy {
	
	struct ViewportInput {
		bool hovered, held, pressed;
	};
	
	ViewportInput detectViewportInput(int mouseButtons);
	
//	bool dragFloat3Pretty(float*, char const* labelID, float speed = 1);
	
	bool dragFloat3Pretty(const char* label,
						  float v[3],
						  float v_speed = 1.0f,
						  float v_min = 0.0f,
						  float v_max = 0.0f,
						  const char* format = "%.3f",
						  ImGuiSliderFlags flags = 0);
	
	auto withID(int id, std::invocable auto&& block) {
		ImGui::PushID(id);
		utl::scope_guard pop = []{
			ImGui::PopID();
		};
		return block();
	}
	
	auto withFont(Font const& font, std::invocable auto&& block) {
		ImGui::PushFont(fonts.get(font));
		utl::scope_guard pop = []{
			ImGui::PopFont();
		};
		return block();
	}
	
	auto withIconFont(IconSize size, std::invocable auto&& block) {
		ImGui::PushFont(icons.font(size));
		utl::scope_guard pop = []{
			ImGui::PopFont();
		};
		return block();
	}
	
	auto withFont(FontWeight w, FontStyle s, std::invocable auto&& block) {
		Font font{};
		font.weight = w;
		font.style = s;
		font.size = FontSize::medium;
		return withFont(font, UTL_FORWARD(block));
	}
	
	auto disabledIf(bool condition, std::invocable auto&& block) {
		ImGui::BeginDisabled(condition);
		utl::scope_guard pop = []{
			ImGui::EndDisabled();
		};
		return block();
	}
	
}

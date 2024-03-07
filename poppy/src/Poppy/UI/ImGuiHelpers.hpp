#pragma once

#include <array>

#include <imgui.h>
#include <utl/scope_guard.hpp>

#include "Poppy/UI/Font.hpp"
#include "Poppy/UI/Icons.hpp"

namespace poppy {

struct ViewportInput {
    bool hovered, held, pressed;
};

ViewportInput detectViewportInput(int mouseButtons);

//	bool dragFloat3Pretty(float*, char const* labelID, float speed = 1);

bool dragFloat3Pretty(char const* label, float v[3], float v_speed = 1.0f,
                      float v_min = 0.0f, float v_max = 0.0f,
                      char const* format = "%.3f", ImGuiSliderFlags flags = 0);

auto withID(int id, std::invocable auto&& block) {
    ImGui::PushID(id);
    utl::scope_guard pop = [] { ImGui::PopID(); };
    return block();
}

auto withFont(Font const& font, std::invocable auto&& block) {
    ImGui::PushFont(fonts.get(font));
    utl::scope_guard pop = [] { ImGui::PopFont(); };
    return block();
}

auto withIconFont(IconSize size, std::invocable auto&& block) {
    ImGui::PushFont(icons.font(size));
    utl::scope_guard pop = [] { ImGui::PopFont(); };
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
    utl::scope_guard pop = [] { ImGui::EndDisabled(); };
    return block();
}

std::array<char, 64> generateUniqueID(std::string_view, int,
                                      bool prepentDoubleHash = false);

template <typename E>
bool enumCombo(E& e, std::size_t count = (std::size_t)E::_count) {
    bool result = false;
    for (size_t j = 0; j < count; ++j) {
        E const i = (E)j;
        bool const selected = e == i;
        auto const label = toString(i);
        bool const pressed = ImGui::Selectable(label.data(), selected);
        //, ImGuiSelectableFlags_SpanAvailWidth);
        if (pressed) {
            e = i;
            result = true;
        }
        if (selected) {
            ImGui::SetItemDefaultFocus();
        }
    }
    return result;
}

void displayEmptyWithReason(
    std::string_view reason,
    Font const& = Font::UIDefault().setWeight(FontWeight::semibold));

} // namespace poppy

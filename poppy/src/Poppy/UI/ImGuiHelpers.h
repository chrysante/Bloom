#ifndef POPPY_UI_IMGUIHELPERS_H
#define POPPY_UI_IMGUIHELPERS_H

#include <array>

#include <imgui.h>
#include <utl/scope_guard.hpp>

#include "Bloom/Core/EnumCount.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/Icons.h"

namespace poppy {

struct ViewportInput {
    bool hovered, held, pressed;
};

ViewportInput detectViewportInput(int mouseButtons);

bool dragFloat3Pretty(char const* label, float v[3], float v_speed = 1.0f,
                      float v_min = 0.0f, float v_max = 0.0f,
                      char const* format = "%.3f", ImGuiSliderFlags flags = 0);

auto withID(int id, std::invocable auto&& block) {
    ImGui::PushID(id);
    utl::scope_guard pop = [] { ImGui::PopID(); };
    return block();
}

auto withFont(FontDesc const& font, std::invocable auto&& block) {
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
    FontDesc font{};
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

std::array<char, 64> generateUniqueID(std::string_view name, int ID,
                                      bool prepentDoubleHash = false);

template <typename E>
bool enumCombo(E& e, std::size_t count = bloom::EnumCount<E>) {
    bool result = false;
    for (size_t j = 0; j < count; ++j) {
        E i = (E)j;
        bool selected = e == i;
        auto label = toString(i);
        bool pressed = ImGui::Selectable(label.data(), selected);
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

template <typename E>
bool enumCombo(E e, std::invocable<E> auto set,
               std::size_t count = bloom::EnumCount<E>) {
    if (enumCombo(e, count)) {
        set(e);
        return true;
    }
    return false;
}

void displayEmptyWithReason(
    std::string_view reason,
    FontDesc const& = FontDesc::UIDefault().setWeight(FontWeight::semibold));

} // namespace poppy

#endif // POPPY_UI_IMGUIHELPERS_H

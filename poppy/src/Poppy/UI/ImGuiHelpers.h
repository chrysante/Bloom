#ifndef POPPY_UI_IMGUIHELPERS_H
#define POPPY_UI_IMGUIHELPERS_H

#include <any>
#include <array>
#include <functional>
#include <initializer_list>
#include <span>
#include <string_view>

#include <imgui.h>
#include <utl/scope_guard.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/EnumCount.h"
#include "Poppy/UI/Font.h"

namespace poppy {

struct ViewportInput {
    bool hovered, held, pressed;
};

ViewportInput detectViewportInput(int mouseButtons);

bool linkButton(bool& linked, vml::float2 size = {},
                IconSize iconSize = IconSize::_16);

struct DragFloat3Options {
    enum LabelStyle { Auto, Wide, Slim };

    float speed = 1.0f;
    float min = 0.0f;
    float max = 0.0f;
    char const* format = "%.3f";
    ImGuiSliderFlags flags = 0;
    bool showLinkButton = false;
    /// Overrides the state of the link button
    std::optional<bool> linkFields;
    LabelStyle labelStyle = Auto;
};

bool dragFloat3Labelled(char const* label, float v[3],
                        DragFloat3Options options = {});

auto withID(int id, std::invocable auto&& block) {
    ImGui::PushID(id);
    utl::scope_guard pop = [] { ImGui::PopID(); };
    return block();
}

decltype(auto) withFont(ImFont* font, std::invocable auto&& block) {
    ImGui::PushFont(font);
    utl::scope_guard pop = [] { ImGui::PopFont(); };
    return block();
}

decltype(auto) withFont(FontDesc const& desc, std::invocable auto&& block) {
    return withFont(FontManager::get(desc), block);
}

decltype(auto) withFont(FontWeight w, FontStyle s,
                        std::invocable auto&& block) {
    FontDesc desc{
        .weight = w,
        .style = s,
        .size = FontSize::Medium,
    };
    return withFont(desc, block);
}

decltype(auto) disabledIf(bool condition, std::invocable auto&& block) {
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

inline constexpr auto DefaultEnumToName = [](auto value) {
    return std::string(magic_enum::enum_name(value));
};

template <typename E, typename ToName = decltype(DefaultEnumToName)>
static std::optional<E> enumCombo2(char const* label, E value, auto&& values,
                                   ToName toName = {}) {
    std::optional<E> newValue;
    if (ImGui::BeginCombo(label, toName(value).data())) {
        for (auto current: values) {
            bool selected = value == current;
            if (ImGui::Selectable(toName(current).data(), selected) &&
                !selected)
            {
                newValue = current;
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return newValue;
}

template <typename E, typename ToName = decltype(DefaultEnumToName)>
static std::optional<E> enumCombo2(char const* label, E value,
                                   ToName toName = {}) {
    return enumCombo2(label, value, magic_enum::enum_values<E>(), toName);
}

/// Placeholder for views that don't display content for good reason
void displayEmptyWithReason(std::string_view reason, vml::float2 size = {});

/// Placeholder for views that don't display content because an error occurred
void displayEmptyWithError(std::string_view reason, vml::float2 size = {});

/// Displays a resizable split view
class SplitView {
public:
    enum Flags { None = 0, NoPadding = 1 << 0 };

    explicit SplitView(std::string ID = "SplitView"): ID(ID) {}

    void display(std::span<std::function<void()> const> contents,
                 Flags flags = None);

    void display(std::initializer_list<std::function<void()>> contents,
                 Flags flags = None);

private:
    std::string ID;
    float totalWidth = 0;
    std::vector<float> widths;
};

/// Extends the lifetime of \p object until the current frame has been rendered
void markObjectRenderHazard(std::any object);

} // namespace poppy

#endif // POPPY_UI_IMGUIHELPERS_H

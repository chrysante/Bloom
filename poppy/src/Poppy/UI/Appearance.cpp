#define UTL_DEFER_MACROS

#include "Poppy/UI/Appearance.h"

#include <span>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/scope_guard.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Core/Yaml.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/SystemAppearance.h"

using namespace mtl::short_types;
using namespace poppy;

static void applyStyle(poppy::StyleDescription const& style) {
    auto& imguiStyle = ImGui::GetStyle();
    auto* colors = imguiStyle.Colors;
    colors[ImGuiCol_Text] = style.colors.text;
    colors[ImGuiCol_TextDisabled] = style.colors.textDisabled;
    colors[ImGuiCol_WindowBg] = style.colors.windowBG;
    colors[ImGuiCol_ChildBg] = style.colors.childWindowBG;
    colors[ImGuiCol_PopupBg] = style.colors.popupBG;
    colors[ImGuiCol_Border] = style.colors.border;
    colors[ImGuiCol_BorderShadow] = {};
    colors[ImGuiCol_FrameBg] = style.colors.controlFrame;
    colors[ImGuiCol_FrameBgHovered] = style.colors.controlFrameHovered;
    colors[ImGuiCol_FrameBgActive] = style.colors.controlFrameDown;
    colors[ImGuiCol_TitleBg] = style.colors.windowTitleBar;
    colors[ImGuiCol_TitleBgActive] = style.colors.activeWindowTitleBar;
    colors[ImGuiCol_TitleBgCollapsed] = style.colors.windowTitleBar;
    colors[ImGuiCol_MenuBarBg] = style.colors.menubarBG;
    colors[ImGuiCol_ScrollbarBg] = {};
    colors[ImGuiCol_ScrollbarGrab] = style.colors.scrollbar;
    colors[ImGuiCol_ScrollbarGrabHovered] = style.colors.scrollbar;
    colors[ImGuiCol_ScrollbarGrabActive] = style.colors.scrollbar;
    colors[ImGuiCol_CheckMark] = style.colors.checkMark;
    colors[ImGuiCol_SliderGrab] = style.colors.highlightControlFrame;
    colors[ImGuiCol_SliderGrabActive] = style.colors.highlightControlFrameDown;
    colors[ImGuiCol_Button] = style.colors.controlFrame;
    colors[ImGuiCol_ButtonHovered] = style.colors.controlFrameHovered;
    colors[ImGuiCol_ButtonActive] = style.colors.controlFrameDown;
    colors[ImGuiCol_Header] = style.colors.highlightControlFrame;
    colors[ImGuiCol_HeaderHovered] = style.colors.highlightControlFrameHovered;
    colors[ImGuiCol_HeaderActive] = style.colors.highlightControlFrameDown;
    colors[ImGuiCol_Separator] = style.colors.separator;
    colors[ImGuiCol_SeparatorHovered] = style.colors.separator;
    colors[ImGuiCol_SeparatorActive] = style.colors.separator;
    colors[ImGuiCol_ResizeGrip] = style.colors.separator;
    colors[ImGuiCol_ResizeGripHovered] = style.colors.separator;
    colors[ImGuiCol_ResizeGripActive] = style.colors.separator;
    colors[ImGuiCol_Tab] = style.colors.tab;
    colors[ImGuiCol_TabHovered] = style.colors.tabHovered;
    colors[ImGuiCol_TabActive] = style.colors.tabActive;
    colors[ImGuiCol_TabUnfocused] = style.colors.tabUnfocused;
    colors[ImGuiCol_TabUnfocusedActive] = style.colors.tabUnfocusedActive;
    colors[ImGuiCol_DockingPreview] = style.colors.dockingPreview;
    colors[ImGuiCol_DockingEmptyBg] = style.colors.windowBG;
    imguiStyle.WindowBorderSize = style.vars.windowBorder ? 1.0 : 0.0;
    imguiStyle.ChildBorderSize = style.vars.childWindowBorder ? 1.0 : 0.0;
    imguiStyle.PopupBorderSize = style.vars.popupBorder ? 1.0 : 0.0;
    imguiStyle.FrameBorderSize = style.vars.frameBorder ? 1.0 : 0.0;
    imguiStyle.TabBorderSize = style.vars.tabBorder ? 1.0 : 0.0;
    imguiStyle.TabBarBorderSize = style.vars.tabBarBorder ? 1.0 : 0.0;
    imguiStyle.WindowRounding = style.vars.windowRounding;
    imguiStyle.ChildRounding = style.vars.childWindowRounding;
    imguiStyle.PopupRounding = style.vars.popupRounding;
    imguiStyle.FrameRounding = style.vars.frameRounding;
    imguiStyle.TabRounding = style.vars.tabRounding;
    imguiStyle.GrabMinSize = style.vars.grabMinSize;
    imguiStyle.GrabRounding = style.vars.grabRounding;
    imguiStyle.ScrollbarSize = style.vars.scrollbarSize;
    imguiStyle.ScrollbarRounding = style.vars.scrollbarRounding;
    imguiStyle.WindowTitleAlign = style.vars.windowTitleAlign;
    imguiStyle.CircleTessellationMaxError = 0.1;
}

static StyleDescription makeStyle(SystemAppearance s) {
    StyleColors colors{
        .text = s.labelColor,
        .textDisabled = s.secondaryLabelColor,
        .windowBG = s.controlBackgroundColor,
        .childWindowBG = s.controlBackgroundColor,
        .popupBG = s.controlBackgroundColor,
        .menubarBG = s.controlBackgroundColor,
        .controlFrame = s.controlColor,
        .controlFrameHovered = s.controlColor,
        .controlFrameDown = s.selectedControlColor,
        .highlightControlFrame = s.controlAccentColor,
        .highlightControlFrameHovered = s.controlAccentColor,
        .highlightControlFrameDown = s.selectedControlColor,
        .checkMark = s.controlAccentColor,
        .windowTitleBar = s.windowBackgroundColor,
        .activeWindowTitleBar = s.windowBackgroundColor,
        .border = s.separatorColor,
        .scrollbar = s.controlColor,
        .separator = s.separatorColor,
        .tab = { 0, 0, 0, 0 },
        .tabHovered = s.selectedControlColor,
        .tabActive = s.selectedControlColor,
        .tabUnfocused = {},
        .tabUnfocusedActive = {},
        .dockingPreview = s.selectedControlColor,
    };
    StyleVars vars{
        .windowBorder = true,
        .childWindowBorder = false,
        .popupBorder = true,
        .frameBorder = true,
        .tabBorder = true,
        .tabBarBorder = true,
        .windowRounding = 4,
        .childWindowRounding = 0,
        .popupRounding = 0,
        .frameRounding = 6,
        .tabRounding = 0,
        .selectableRounding = {},
        .grabMinSize = 12,
        .grabRounding = 6,
        .scrollbarSize = 10,
        .scrollbarRounding = 10,
        .windowTitleAlign = { 0.5, 0.5 },
    };
    return StyleDescription{ colors, vars };
}

Appearance::Appearance() { update(/* force = */ true); }

void Appearance::update(bool force) {
    auto currentType = SystemAppearance::getCurrentType();
    if (!force && currentType == mType) {
        return;
    }
    auto systemAppearance = SystemAppearance::get(currentType);
    mType = currentType;
    globalHue = mtl::rgb_to_hsv(systemAppearance.controlAccentColor.rgb).hue;
    mStyle = makeStyle(systemAppearance);
    applyStyle(mStyle);
}

static void showWarning() {
    ImGui::PushFont(fonts.get(
        FontDesc{ .size = FontSize::large, .weight = FontWeight::bold }));
    ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0, 1, 1 });
    ImGui::TextWrapped(
        "Warning: Edits need to be made in code in Appearance.cpp to be persistent");
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void Appearance::showInspector(bool* open) {
    if (!ImGui::Begin("Style Editor", open)) {
        return;
    }
    utl_defer { ImGui::End(); };
    showWarning();
    auto& style = staticInstance().mStyle;
    bool update = false;
    if (!ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        return;
    }
    utl_defer { ImGui::EndTabBar(); };
    if (ImGui::BeginTabItem("Sizes")) {
        utl_defer { ImGui::EndTabItem(); };
        ImGui::Text("Borders");
        auto& vars = style.vars;
        update |= ImGui::Checkbox("Window Border", &vars.windowBorder);
        update |=
            ImGui::Checkbox("Child Window Border", &vars.childWindowBorder);
        update |= ImGui::Checkbox("Popup Border", &vars.popupBorder);
        update |= ImGui::Checkbox("Frame Border", &vars.frameBorder);
        update |= ImGui::Checkbox("Tab Border", &vars.tabBorder);
        update |= ImGui::Checkbox("Tab Bar Border", &vars.tabBarBorder);
        ImGui::Text("Roundings");
        update |= ImGui::SliderFloat("Window Rounding", &vars.windowRounding, 0,
                                     12, "%.0f");
        update |= ImGui::SliderFloat("Child Window Rounding",
                                     &vars.childWindowRounding, 0, 12, "%.0f");
        update |= ImGui::SliderFloat("Popup Rounding", &vars.popupRounding, 0,
                                     12, "%.0f");
        update |= ImGui::SliderFloat("Frame Rounding", &vars.frameRounding, 0,
                                     12, "%.0f");
        update |= ImGui::SliderFloat("Tab Rounding", &vars.tabRounding, 0, 12,
                                     "%.0f");
        update |= ImGui::SliderFloat("Selectable Rounding",
                                     &vars.selectableRounding, 0, 12, "%.0f");
        ImGui::Text("Grab");
        update |= ImGui::SliderFloat("Grab Min Size", &vars.grabMinSize, 1, 20,
                                     "%.0f");
        update |= ImGui::SliderFloat("Grab Rounding", &vars.grabRounding, 0, 12,
                                     "%.1f");
        ImGui::Text("Scrollbar");
        update |= ImGui::SliderFloat("Scrollbar Size", &vars.scrollbarSize, 1,
                                     20, "%.0f");
        update |= ImGui::SliderFloat("Scrollbar Rounding",
                                     &vars.scrollbarRounding, 0, 12, "%.0f");
        ImGui::Text("Misc");
        update |= ImGui::SliderFloat2("Window Title Align",
                                      vars.windowTitleAlign.data(), 0.0f, 1.0f,
                                      "%.2f");
    }
    if (ImGui::BeginTabItem("Colors")) {
        utl_defer { ImGui::EndTabItem(); };
        auto item = [&](char const* name, mtl::float4& color) -> bool {
            auto hsva = mtl::rgba_to_hsva(color);
            bool result =
                ImGui::ColorEdit4(name, hsva.data(),
                                  ImGuiColorEditFlags_AlphaPreviewHalf |
                                      ImGuiColorEditFlags_NoInputs |
                                      ImGuiColorEditFlags_Float |
                                      ImGuiColorEditFlags_InputHSV);
            color = mtl::hsva_to_rgba(hsva);
            return result;
        };
        if (ImGui::BeginChild("##Colors")) {
            utl_defer { ImGui::EndChild(); };
            auto& colors = style.colors;
            update |= item("text", colors.text);
            update |= item("textDisabled", colors.textDisabled);
            update |= item("windowBG", colors.windowBG);
            update |= item("childWindowBG", colors.childWindowBG);
            update |= item("popupBG", colors.popupBG);
            update |= item("menubarBG", colors.menubarBG);
            update |= item("controlFrame", colors.controlFrame);
            update |= item("controlFrameHovered", colors.controlFrameHovered);
            update |= item("controlFrameDown", colors.controlFrameDown);
            update |=
                item("highlightControlFrame", colors.highlightControlFrame);
            update |= item("highlightControlFrameHovered",
                           colors.highlightControlFrameHovered);
            update |= item("highlightControlFrameDown",
                           colors.highlightControlFrameDown);
            update |= item("checkMark", colors.checkMark);
            update |= item("windowTitleBar", colors.windowTitleBar);
            update |= item("activeWindowTitleBar", colors.activeWindowTitleBar);
            update |= item("border", colors.border);
            update |= item("scrollbar", colors.scrollbar);
            update |= item("separator", colors.separator);
            update |= item("tab", colors.tab);
            update |= item("tabActive", colors.tabActive);
            update |= item("tabUnfocused", colors.tabUnfocused);
            update |= item("tabUnfocusedActive", colors.tabUnfocusedActive);
            update |= item("dockingPreview", colors.dockingPreview);
        }
    }
    if (update) {
        applyStyle(style);
    }
}

static bool colorPreview(float4& color, char const* label) {
    ImGuiColorEditFlags flags = 0;
    flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar |
             ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaPreviewHalf;
    char id[16] = "##";
    strncpy(&id[2], label, 13);
    bool const result = ImGui::ColorEdit4(id, color.data(), flags);
    ImGui::SameLine();
    ImGui::Text("%s", label);
    return result;
}

void poppy::systemStyleInspector(bool* open) {
#define COLOR_PREVIEW(NAME)                                                    \
    do {                                                                       \
        mtl::float4 color = a.NAME;                                            \
        colorPreview(color, #NAME);                                            \
    } while (0)
    SystemAppearance a = SystemAppearance::getCurrent();
    ImGui::Begin("System Style Colors", open);
    COLOR_PREVIEW(labelColor);
    COLOR_PREVIEW(secondaryLabelColor);
    COLOR_PREVIEW(tertiaryLabelColor);
    COLOR_PREVIEW(quaternaryLabelColor);
    COLOR_PREVIEW(textColor);
    COLOR_PREVIEW(placeholderTextColor);
    COLOR_PREVIEW(selectedTextColor);
    COLOR_PREVIEW(textBackgroundColor);
    COLOR_PREVIEW(selectedTextBackgroundColor);
    COLOR_PREVIEW(keyboardFocusIndicatorColor);
    COLOR_PREVIEW(unemphasizedSelectedTextColor);
    COLOR_PREVIEW(unemphasizedSelectedTextBackgroundColor);
    COLOR_PREVIEW(linkColor);
    COLOR_PREVIEW(separatorColor);
    COLOR_PREVIEW(selectedContentBackgroundColor);
    COLOR_PREVIEW(unemphasizedSelectedContentBackgroundColor);
    COLOR_PREVIEW(selectedMenuItemTextColor);
    COLOR_PREVIEW(gridColor);
    COLOR_PREVIEW(headerTextColor);
    COLOR_PREVIEW(alternatingContentBackgroundColors[0]);
    COLOR_PREVIEW(alternatingContentBackgroundColors[1]);
    COLOR_PREVIEW(controlAccentColor);
    COLOR_PREVIEW(controlColor);
    COLOR_PREVIEW(controlBackgroundColor);
    COLOR_PREVIEW(controlTextColor);
    COLOR_PREVIEW(disabledControlTextColor);
    COLOR_PREVIEW(selectedControlColor);
    COLOR_PREVIEW(selectedControlTextColor);
    COLOR_PREVIEW(alternateSelectedControlTextColor);
    COLOR_PREVIEW(scrubberTexturedBackground);
    COLOR_PREVIEW(windowBackgroundColor);
    COLOR_PREVIEW(windowFrameTextColor);
    COLOR_PREVIEW(underPageBackgroundColor);
    COLOR_PREVIEW(findHighlightColor);
    COLOR_PREVIEW(highlightColor);
    COLOR_PREVIEW(shadowColor);
#undef COLOR_PICKER
    ImGui::End();
}

Appearance& Appearance::staticInstance() {
    static Appearance inst;
    return inst;
}

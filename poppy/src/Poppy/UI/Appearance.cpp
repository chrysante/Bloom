#include "Poppy/UI/Appearance.hpp"

#include <span>

#include <imgui.h>
#include <imgui_internal.h>
#include <yaml-cpp/helpers.hpp>
#include <yaml-cpp/yaml.h>

#include "Poppy/Core/Debug.hpp"
#include "Poppy/UI/SystemAppearance.hpp"

using namespace mtl::short_types;
using namespace poppy;

static void applyStyle(poppy::Style const& style) {
    auto& imguiStyle                      = ImGui::GetStyle();
    auto* const colors                    = imguiStyle.Colors;
    constexpr mtl::float4 clear           = 0;
    colors[ImGuiCol_Text]                 = style.colors.text;
    colors[ImGuiCol_TextDisabled]         = style.colors.textDisabled;
    colors[ImGuiCol_WindowBg]             = style.colors.windowBG;
    colors[ImGuiCol_ChildBg]              = style.colors.childWindowBG;
    colors[ImGuiCol_PopupBg]              = style.colors.popupBG;
    colors[ImGuiCol_Border]               = style.colors.border;
    colors[ImGuiCol_BorderShadow]         = clear;
    colors[ImGuiCol_FrameBg]              = style.colors.controlFrame;
    colors[ImGuiCol_FrameBgHovered]       = style.colors.controlFrameHovered;
    colors[ImGuiCol_FrameBgActive]        = style.colors.controlFrameDown;
    colors[ImGuiCol_TitleBg]              = style.colors.windowTitleBar;
    colors[ImGuiCol_TitleBgActive]        = style.colors.activeWindowTitleBar;
    colors[ImGuiCol_TitleBgCollapsed]     = style.colors.windowTitleBar;
    colors[ImGuiCol_MenuBarBg]            = style.colors.menubarBG;
    colors[ImGuiCol_ScrollbarBg]          = clear;
    colors[ImGuiCol_ScrollbarGrab]        = style.colors.scrollbar;
    colors[ImGuiCol_ScrollbarGrabHovered] = style.colors.scrollbar;
    colors[ImGuiCol_ScrollbarGrabActive]  = style.colors.scrollbar;
    colors[ImGuiCol_CheckMark]            = style.colors.checkMark;
    colors[ImGuiCol_SliderGrab]           = style.colors.highlightControlFrame;
    colors[ImGuiCol_SliderGrabActive] = style.colors.highlightControlFrameDown;
    colors[ImGuiCol_Button]           = style.colors.controlFrame;
    colors[ImGuiCol_ButtonHovered]    = style.colors.controlFrameHovered;
    colors[ImGuiCol_ButtonActive]     = style.colors.controlFrameDown;
    colors[ImGuiCol_Header]           = style.colors.highlightControlFrame;
    colors[ImGuiCol_HeaderHovered] = style.colors.highlightControlFrameHovered;
    colors[ImGuiCol_HeaderActive]  = style.colors.highlightControlFrameDown;
    colors[ImGuiCol_Separator]     = style.colors.separator;
    colors[ImGuiCol_SeparatorHovered]   = style.colors.separator;
    colors[ImGuiCol_SeparatorActive]    = style.colors.separator;
    colors[ImGuiCol_ResizeGrip]         = clear;
    colors[ImGuiCol_ResizeGripHovered]  = clear;
    colors[ImGuiCol_ResizeGripActive]   = clear;
    colors[ImGuiCol_Tab]                = style.colors.tab;
    colors[ImGuiCol_TabActive]          = style.colors.tabActive;
    colors[ImGuiCol_TabUnfocused]       = style.colors.tabUnfocused;
    colors[ImGuiCol_TabUnfocusedActive] = style.colors.tabUnfocusedActive;
    colors[ImGuiCol_DockingPreview]     = style.colors.dockingPreview;
    colors[ImGuiCol_DockingEmptyBg]     = style.colors.windowBG;
    imguiStyle.WindowBorderSize         = (float)style.vars.windowBorder;
    imguiStyle.ChildBorderSize          = (float)style.vars.childWindowBorder;
    imguiStyle.PopupBorderSize          = (float)style.vars.popupBorder;
    imguiStyle.FrameBorderSize          = (float)style.vars.frameBorder;
    imguiStyle.TabBorderSize            = (float)style.vars.tabBorder;
    imguiStyle.WindowRounding           = style.vars.windowRounding;
    imguiStyle.ChildRounding            = style.vars.childWindowRounding;
    imguiStyle.PopupRounding            = style.vars.popupRounding;
    imguiStyle.FrameRounding            = style.vars.frameRounding;
    imguiStyle.SelectableRounding       = style.vars.selectableRounding;
    imguiStyle.TabRounding              = style.vars.tabRounding;
    imguiStyle.GrabMinSize              = style.vars.grabMinSize;
    imguiStyle.GrabRounding             = style.vars.grabRounding;
    imguiStyle.ScrollbarSize     = std::max(1.0f, style.vars.scrollbarSize);
    imguiStyle.ScrollbarRounding = style.vars.scrollbarRounding;
    imguiStyle.WindowTitleAlign  = { style.vars.windowTitleAlign, 0.5 };
    imguiStyle.CircleTessellationMaxError = 0.1;
}

Appearance poppy::appearance{};

StyleColors poppy::makeStyle(float hue, StyleColorDescription const& desc) {
    StyleColors result{};
    result.text                 = mtl::hsva_to_rgba(desc.text);
    result.textDisabled         = mtl::hsva_to_rgba(desc.textDisabled);
    result.windowBG             = mtl::hsva_to_rgba(desc.windowBG);
    result.childWindowBG        = mtl::hsva_to_rgba(desc.windowBG);
    result.popupBG              = mtl::hsva_to_rgba(desc.popupBG);
    result.menubarBG            = mtl::hsva_to_rgba(desc.menubarBG);
    result.border               = mtl::hsva_to_rgba(desc.border);
    result.windowTitleBar       = mtl::hsva_to_rgba(desc.windowTitleBar);
    result.activeWindowTitleBar = mtl::hsva_to_rgba(desc.activeWindowTitleBar);
    result.scrollbar            = mtl::hsva_to_rgba(desc.scrollbar);
    result.separator            = mtl::hsva_to_rgba(desc.separator);
    result.dockingPreview       = mtl::hsva_to_rgba(desc.dockPreview);
    result.controlFrame         = mtl::hsva_to_rgba(desc.controlFrame);
    result.controlFrameHovered  = mtl::hsva_to_rgba(desc.controlFrameHovered);
    result.controlFrameDown     = mtl::hsva_to_rgba(desc.controlFrameDown);
    result.highlightControlFrame =
        mtl::hsva_to_rgba(desc.highlightControlFrame);
    result.highlightControlFrameHovered =
        mtl::hsva_to_rgba(desc.highlightControlFrameHovered);
    result.highlightControlFrameDown =
        mtl::hsva_to_rgba(desc.highlightControlFrameDown);
    result.checkMark          = mtl::hsva_to_rgba(desc.checkMark);
    result.tab                = mtl::hsva_to_rgba(desc.tab);
    result.tabActive          = mtl::hsva_to_rgba(desc.tabActive);
    result.tabUnfocused       = mtl::hsva_to_rgba(desc.tabUnfocused);
    result.tabUnfocusedActive = mtl::hsva_to_rgba(desc.tabUnfocusedActive);
    return result;
}

Appearance::Appearance() { update(); }

void Appearance::update(bool force) {
    auto const currentType = SystemAppearance::getCurrentType();
    if (!force && currentType == mType) {
        return;
    }
    auto const systemAppearance = SystemAppearance::get(currentType);
    mType                       = currentType;
    globalHue = mtl::rgb_to_hsv(systemAppearance.controlAccentColor.rgb).hue;
    switch (currentType) {
    case AppearanceType::light:
        mStyle.colors = makeStyle(globalHue, lightColorDesc);
        applyStyle(mStyle);
        break;

    case AppearanceType::dark:
        mStyle.colors = makeStyle(globalHue, darkColorDesc);
        applyStyle(mStyle);
        break;

    default:
        break;
    }
}

void Appearance::deserialize(YAML::Node root) {
    try {
        lightColorDesc =
            root["Light Color Description"].as<StyleColorDescription>();
        darkColorDesc =
            root["Dark Color Description"].as<StyleColorDescription>();
        mStyle.vars = root["Style Vars"].as<StyleVars>();
    }
    catch (...) {
        Logger::error("Failed to load Appearance");
    }

    update();
}

YAML::Node Appearance::serialize() const {
    YAML::Node root;

    root["Light Color Description"] = lightColorDesc;
    root["Dark Color Description"]  = darkColorDesc;
    root["Style Vars"]              = mStyle.vars;

    return root;
}

void Appearance::showInspector(bool* open) {
    ImGui::Begin("Style Editor");

    bool update = false;

    char const* const names[2] = { "Dark", "Light" };
    update |= ImGui::Combo("Style", (int*)&mType, names, 2);

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Sizes")) {
            ImGui::Text("Borders");
            update |=
                ImGui::Checkbox("Window Border", &mStyle.vars.windowBorder);
            update |= ImGui::Checkbox("Child Window Border",
                                      &mStyle.vars.childWindowBorder);
            update |= ImGui::Checkbox("Popup Border", &mStyle.vars.popupBorder);
            update |= ImGui::Checkbox("Frame Border", &mStyle.vars.frameBorder);
            update |= ImGui::Checkbox("Tab Border", &mStyle.vars.tabBorder);

            ImGui::Text("Roundings");
            update |= ImGui::SliderFloat("Window Rounding",
                                         &mStyle.vars.windowRounding,
                                         0,
                                         12,
                                         "%.0f");
            update |= ImGui::SliderFloat("Child Window Rounding",
                                         &mStyle.vars.childWindowRounding,
                                         0,
                                         12,
                                         "%.0f");
            update |= ImGui::SliderFloat("Popup Rounding",
                                         &mStyle.vars.popupRounding,
                                         0,
                                         12,
                                         "%.0f");
            update |= ImGui::SliderFloat("Frame Rounding",
                                         &mStyle.vars.frameRounding,
                                         0,
                                         12,
                                         "%.0f");
            update |= ImGui::SliderFloat("Tab Rounding",
                                         &mStyle.vars.tabRounding,
                                         0,
                                         12,
                                         "%.0f");
            update |= ImGui::SliderFloat("Selectable Rounding",
                                         &mStyle.vars.selectableRounding,
                                         0,
                                         12,
                                         "%.0f");

            ImGui::Text("Grab");
            update |= ImGui::SliderFloat("Grab Min Size",
                                         &mStyle.vars.grabMinSize,
                                         1,
                                         20,
                                         "%.0f");
            update |= ImGui::SliderFloat("Grab Rounding",
                                         &mStyle.vars.grabRounding,
                                         0,
                                         12,
                                         "%.1f");

            ImGui::Text("Scrollbar");
            update |= ImGui::SliderFloat("Scrollbar Size",
                                         &mStyle.vars.scrollbarSize,
                                         1,
                                         20,
                                         "%.0f");
            update |= ImGui::SliderFloat("Scrollbar Rounding",
                                         &mStyle.vars.scrollbarRounding,
                                         0,
                                         12,
                                         "%.0f");

            ImGui::Text("Misc");
            update |= ImGui::SliderFloat2("Window Title Align",
                                          &mStyle.vars.windowTitleAlign,
                                          0.0f,
                                          1.0f,
                                          "%.2f");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Colors")) {
            auto item = [&](char const* name, UIColor& color) -> bool {
                bool const result =
                    ImGui::ColorEdit4(name,
                                      color.data(),
                                      ImGuiColorEditFlags_AlphaPreviewHalf |
                                          ImGuiColorEditFlags_NoInputs |
                                          ImGuiColorEditFlags_Float |
                                          ImGuiColorEditFlags_InputHSV);
                color.hue = globalHue;
                return result;
            };

            ImGui::BeginChild("##Colors");

            auto& desc =
                mType == AppearanceType::light ? lightColorDesc : darkColorDesc;

            update |= item("Control Frame", desc.controlFrame);
            update |= item("Control Frame Hovered", desc.controlFrameHovered);
            update |= item("Control Frame Down", desc.controlFrameDown);
            update |=
                item("Highlight Control Frame", desc.highlightControlFrame);
            update |= item("Highlight Control Frame Hovered",
                           desc.highlightControlFrameHovered);
            update |= item("Highlight Control Frame Down",
                           desc.highlightControlFrameDown);
            update |= item("Check Mark", desc.checkMark);
            update |= item("Tab", desc.tab);
            update |= item("Tab Active", desc.tabActive);
            update |= item("Tab Unfocused", desc.tabUnfocused);
            update |= item("Tab Unfocused Active", desc.tabUnfocusedActive);
            update |= item("Text", desc.text);
            update |= item("Text Disabled", desc.textDisabled);
            update |= item("Window Background", desc.windowBG);
            update |= item("Popup Background", desc.popupBG);
            update |= item("Menubar Background", desc.menubarBG);
            update |= item("Border", desc.border);
            update |= item("Window Titlebar", desc.windowTitleBar);
            update |= item("Active Window Titlebar", desc.activeWindowTitleBar);
            update |= item("Separator", desc.separator);
            update |= item("Scrollbar", desc.scrollbar);
            update |= item("Dock Preview", desc.dockPreview);

            ImGui::EndChild();

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    if (update) {
        this->update(true);
    }
}

static bool colorEdit(float4& color, char const* label) {
    ImGuiColorEditFlags flags = 0;
    flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar |
             ImGuiColorEditFlags_Float;

    char id[16] = "##";
    strncpy(&id[2], label, 13);
    bool const result = ImGui::ColorEdit4(id, color.data(), flags);

    ImGui::SameLine();
    ImGui::Text("%s", label);

    return result;
}

void poppy::systemStyleInspector(bool* open) {
    SystemAppearance a = SystemAppearance::getCurrent();
    ImGui::Begin("System Style Colors", open);

#define COLOR_PICKER(NAME)                                                     \
    do {                                                                       \
        mtl::float4 color = a.NAME;                                            \
        colorEdit(color, #NAME);                                               \
    } while (0)

    COLOR_PICKER(labelColor);
    COLOR_PICKER(secondaryLabelColor);
    COLOR_PICKER(tertiaryLabelColor);
    COLOR_PICKER(quaternaryLabelColor);
    COLOR_PICKER(textColor);
    COLOR_PICKER(placeholderTextColor);
    COLOR_PICKER(selectedTextColor);
    COLOR_PICKER(textBackgroundColor);
    COLOR_PICKER(selectedTextBackgroundColor);
    COLOR_PICKER(keyboardFocusIndicatorColor);
    COLOR_PICKER(unemphasizedSelectedTextColor);
    COLOR_PICKER(unemphasizedSelectedTextBackgroundColor);
    COLOR_PICKER(linkColor);
    COLOR_PICKER(separatorColor);
    COLOR_PICKER(selectedContentBackgroundColor);
    COLOR_PICKER(unemphasizedSelectedContentBackgroundColor);
    COLOR_PICKER(selectedMenuItemTextColor);
    COLOR_PICKER(gridColor);
    COLOR_PICKER(headerTextColor);
    COLOR_PICKER(alternatingContentBackgroundColors[0]);
    COLOR_PICKER(alternatingContentBackgroundColors[1]);
    COLOR_PICKER(controlAccentColor);
    COLOR_PICKER(controlColor);
    COLOR_PICKER(controlBackgroundColor);
    COLOR_PICKER(controlTextColor);
    COLOR_PICKER(disabledControlTextColor);
    //		COLOR_PICKER(currentControlTint);
    COLOR_PICKER(selectedControlColor);
    COLOR_PICKER(selectedControlTextColor);
    COLOR_PICKER(alternateSelectedControlTextColor);
    COLOR_PICKER(scrubberTexturedBackground);
    COLOR_PICKER(windowBackgroundColor);
    COLOR_PICKER(windowFrameTextColor);
    COLOR_PICKER(underPageBackgroundColor);
    COLOR_PICKER(findHighlightColor);
    COLOR_PICKER(highlightColor);
    COLOR_PICKER(shadowColor);

#undef COLOR_PICKER

    ImGui::End();
}

#if 0
	void setStyleColorsLight() {
		ImVec4* colors = ImGui::GetStyle().Colors;
	
		colors[ImGuiCol_Text]                  = {};
		colors[ImGuiCol_TextDisabled]          = {};
		colors[ImGuiCol_WindowBg]              = {};
		colors[ImGuiCol_ChildBg]               = {};
		colors[ImGuiCol_PopupBg]               = {};
		colors[ImGuiCol_Border]                = {};
		colors[ImGuiCol_BorderShadow]          = {};
		colors[ImGuiCol_FrameBg]               = {};
		colors[ImGuiCol_FrameBgHovered]        = {};
		colors[ImGuiCol_FrameBgActive]         = {};
		colors[ImGuiCol_TitleBg]               = {};
		colors[ImGuiCol_TitleBgActive]         = {};
		colors[ImGuiCol_TitleBgCollapsed]      = {};
		colors[ImGuiCol_MenuBarBg]             = {};
		colors[ImGuiCol_ScrollbarBg]           = {};
		colors[ImGuiCol_ScrollbarGrab]         = {};
		colors[ImGuiCol_ScrollbarGrabHovered]  = {};
		colors[ImGuiCol_ScrollbarGrabActive]   = {};
		colors[ImGuiCol_CheckMark]             = {};
		colors[ImGuiCol_SliderGrab]            = {};
		colors[ImGuiCol_SliderGrabActive]      = {};
		colors[ImGuiCol_Button]                = {};
		colors[ImGuiCol_ButtonHovered]         = {};
		colors[ImGuiCol_ButtonActive]          = {};
		colors[ImGuiCol_Header]                = {};
		colors[ImGuiCol_HeaderHovered]         = {};
		colors[ImGuiCol_HeaderActive]          = {};
		colors[ImGuiCol_Separator]             = {};
		colors[ImGuiCol_SeparatorHovered]      = {};
		colors[ImGuiCol_SeparatorActive]       = {};
		colors[ImGuiCol_ResizeGrip]            = {};
		colors[ImGuiCol_ResizeGripHovered]     = {};
		colors[ImGuiCol_ResizeGripActive]      = {};
		colors[ImGuiCol_Tab]                   = {};
		colors[ImGuiCol_TabHovered]            = {};
		colors[ImGuiCol_TabActive]             = {};
		colors[ImGuiCol_TabUnfocused]          = {};
		colors[ImGuiCol_TabUnfocusedActive]    = {};
		colors[ImGuiCol_DockingPreview]        = {};
		colors[ImGuiCol_DockingEmptyBg]        = {};
		colors[ImGuiCol_PlotLines]             = {};
		colors[ImGuiCol_PlotLinesHovered]      = {};
		colors[ImGuiCol_PlotHistogram]         = {};
		colors[ImGuiCol_PlotHistogramHovered]  = {};
		colors[ImGuiCol_TableHeaderBg]         = {};
		colors[ImGuiCol_TableBorderStrong]     = {};
		colors[ImGuiCol_TableBorderLight]      = {};
		colors[ImGuiCol_TableRowBg]            = {};
		colors[ImGuiCol_TableRowBgAlt]         = {};
		colors[ImGuiCol_TextSelectedBg]        = {};
		colors[ImGuiCol_DragDropTarget]        = {};
		colors[ImGuiCol_NavHighlight]          = {};
		colors[ImGuiCol_NavWindowingHighlight] = {};
		colors[ImGuiCol_NavWindowingDimBg]     = {};
		colors[ImGuiCol_ModalWindowDimBg]      = {};
	}
	
	void setStyleColorsDark() {
		ImVec4* colors = ImGui::GetStyle().Colors;
		Appearance const a = Appearance::get(Appearance::Type::dark);
		
		colors[ImGuiCol_Text]                  = a.labelColor;
		colors[ImGuiCol_TextDisabled]          = a.tertiaryLabelColor;
		colors[ImGuiCol_WindowBg]              = a.controlBackgroundColor;
		colors[ImGuiCol_ChildBg]               = {};
		colors[ImGuiCol_PopupBg]               = {};
		colors[ImGuiCol_Border]                = {};
		colors[ImGuiCol_BorderShadow]          = {};
		colors[ImGuiCol_FrameBg]               = {};
		colors[ImGuiCol_FrameBgHovered]        = {};
		colors[ImGuiCol_FrameBgActive]         = {};
		colors[ImGuiCol_TitleBg]               = {};
		colors[ImGuiCol_TitleBgActive]         = {};
		colors[ImGuiCol_TitleBgCollapsed]      = {};
		colors[ImGuiCol_MenuBarBg]             = {};
		colors[ImGuiCol_ScrollbarBg]           = {};
		colors[ImGuiCol_ScrollbarGrab]         = {};
		colors[ImGuiCol_ScrollbarGrabHovered]  = {};
		colors[ImGuiCol_ScrollbarGrabActive]   = {};
		colors[ImGuiCol_CheckMark]             = {};
		colors[ImGuiCol_SliderGrab]            = {};
		colors[ImGuiCol_SliderGrabActive]      = {};
		colors[ImGuiCol_Button]                = {};
		colors[ImGuiCol_ButtonHovered]         = {};
		colors[ImGuiCol_ButtonActive]          = {};
		colors[ImGuiCol_Header]                = {};
		colors[ImGuiCol_HeaderHovered]         = {};
		colors[ImGuiCol_HeaderActive]          = {};
		colors[ImGuiCol_Separator]             = {};
		colors[ImGuiCol_SeparatorHovered]      = {};
		colors[ImGuiCol_SeparatorActive]       = {};
		colors[ImGuiCol_ResizeGrip]            = {};
		colors[ImGuiCol_ResizeGripHovered]     = {};
		colors[ImGuiCol_ResizeGripActive]      = {};
		colors[ImGuiCol_Tab]                   = {};
		colors[ImGuiCol_TabHovered]            = {};
		colors[ImGuiCol_TabActive]             = {};
		colors[ImGuiCol_TabUnfocused]          = {};
		colors[ImGuiCol_TabUnfocusedActive]    = {};
		colors[ImGuiCol_DockingPreview]        = {};
		colors[ImGuiCol_DockingEmptyBg]        = {};
		colors[ImGuiCol_PlotLines]             = {};
		colors[ImGuiCol_PlotLinesHovered]      = {};
		colors[ImGuiCol_PlotHistogram]         = {};
		colors[ImGuiCol_PlotHistogramHovered]  = {};
		colors[ImGuiCol_TableHeaderBg]         = {};
		colors[ImGuiCol_TableBorderStrong]     = {};
		colors[ImGuiCol_TableBorderLight]      = {};
		colors[ImGuiCol_TableRowBg]            = {};
		colors[ImGuiCol_TableRowBgAlt]         = {};
		colors[ImGuiCol_TextSelectedBg]        = {};
		colors[ImGuiCol_DragDropTarget]        = {};
		colors[ImGuiCol_NavHighlight]          = {};
		colors[ImGuiCol_NavWindowingHighlight] = {};
		colors[ImGuiCol_NavWindowingDimBg]     = {};
		colors[ImGuiCol_ModalWindowDimBg]      = {};
	}
	
	void setStyleColorsPrefered() {
		
	}
	
    /// MARK: Debug
	
	
	void StyleColorsView(bool* open) {
		using namespace ImGui;
		
		ImVec4* colors = ImGui::GetStyle().Colors;
		
		static bool showAdvanced = true;
		
		auto entry = [&](bool advanced, ImGuiCol index, char const* label) {
			if (advanced && !showAdvanced) { return; }
			colorEdit((float*)&colors[index], label);
		};
		
		Begin("Style Colors");
		
		entry(false, ImGuiCol_Text, "Text");
		entry(false, ImGuiCol_TextDisabled, "TextDisabled");
		entry(false, ImGuiCol_WindowBg, "WindowBg");
		entry(true, ImGuiCol_ChildBg, "ChildBg");
		entry(true, ImGuiCol_PopupBg, "PopupBg");
		entry(false, ImGuiCol_Border, "Border");
		entry(false, ImGuiCol_BorderShadow, "BorderShadow");
		entry(false, ImGuiCol_FrameBg, "FrameBg");
		entry(false, ImGuiCol_FrameBgHovered, "FrameBgHovered");
		entry(false, ImGuiCol_FrameBgActive, "FrameBgActive");
		entry(false, ImGuiCol_TitleBg, "TitleBg");
		entry(false, ImGuiCol_TitleBgActive, "TitleBgActive");
		entry(false, ImGuiCol_TitleBgCollapsed, "TitleBgCollapsed");
		entry(false, ImGuiCol_MenuBarBg, "MenuBarBg");
		entry(true, ImGuiCol_ScrollbarBg, "ScrollbarBg");
		entry(true, ImGuiCol_ScrollbarGrab, "ScrollbarGrab");
		entry(true, ImGuiCol_ScrollbarGrabHovered, "ScrollbarGrabHovered");
		entry(true, ImGuiCol_ScrollbarGrabActive, "ScrollbarGrabActive");
		entry(false, ImGuiCol_CheckMark, "CheckMark");
		entry(true, ImGuiCol_SliderGrab, "SliderGrab");
		entry(true, ImGuiCol_SliderGrabActive, "SliderGrabActive");
		entry(true, ImGuiCol_Button, "Button");
		entry(true, ImGuiCol_ButtonHovered, "ButtonHovered");
		entry(true, ImGuiCol_ButtonActive, "ButtonActive");
		entry(true, ImGuiCol_Header, "Header");
		entry(true, ImGuiCol_HeaderHovered, "HeaderHovered");
		entry(true, ImGuiCol_HeaderActive, "HeaderActive");
		entry(false, ImGuiCol_Separator, "Separator");
		entry(true, ImGuiCol_SeparatorHovered, "SeparatorHovered");
		entry(true, ImGuiCol_SeparatorActive, "SeparatorActive");
		entry(true, ImGuiCol_ResizeGrip, "ResizeGrip");
		entry(true, ImGuiCol_ResizeGripHovered, "ResizeGripHovered");
		entry(true, ImGuiCol_ResizeGripActive, "ResizeGripActive");
		entry(false, ImGuiCol_Tab, "Tab");
		entry(false, ImGuiCol_TabHovered, "TabHovered");
		entry(false, ImGuiCol_TabActive, "TabActive");
		entry(false, ImGuiCol_TabUnfocused, "TabUnfocused");
		entry(false, ImGuiCol_TabUnfocusedActive, "TabUnfocusedActive");
		entry(true, ImGuiCol_DockingPreview, "DockingPreview");
		entry(true, ImGuiCol_DockingEmptyBg, "DockingEmptyBg");
		entry(false, ImGuiCol_PlotLines, "PlotLines");
		entry(false, ImGuiCol_PlotLinesHovered, "PlotLinesHovered");
		entry(false, ImGuiCol_PlotHistogram, "PlotHistogram");
		entry(false, ImGuiCol_PlotHistogramHovered, "PlotHistogramHovered");
		entry(false, ImGuiCol_TableHeaderBg, "TableHeaderBg");
		entry(false, ImGuiCol_TableBorderStrong, "TableBorderStrong");
		entry(false, ImGuiCol_TableBorderLight, "TableBorderLight");
		entry(false, ImGuiCol_TableRowBg, "TableRowBg");
		entry(false, ImGuiCol_TableRowBgAlt, "TableRowBgAlt");
		entry(false, ImGuiCol_TextSelectedBg, "TextSelectedBg");
		entry(false, ImGuiCol_DragDropTarget, "DragDropTarget");
		entry(false, ImGuiCol_NavHighlight, "NavHighlight");
		entry(false, ImGuiCol_NavWindowingHighlight, "NavWindowingHighlight");
		entry(false, ImGuiCol_NavWindowingDimBg, "NavWindowingDimBg");
		entry(false, ImGuiCol_ModalWindowDimBg, "ModalWindowDimBg");
		
		End();
		
//		colors[ImGuiCol_ChildBg] = colors[ImGuiCol_WindowBg];
//		colors[ImGuiCol_PopupBg] = colors[ImGuiCol_WindowBg];
//
//		colors[ImGuiCol_Button] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgHovered];
//		colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_FrameBgActive];
//
//		colors[ImGuiCol_ScrollbarBg] = { 0, 0, 0, 0 };
//		colors[ImGuiCol_ScrollbarGrab] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_ScrollbarGrabHovered] = colors[ImGuiCol_FrameBgHovered];
//
//		colors[ImGuiCol_SliderGrab] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_SliderGrabActive] = colors[ImGuiCol_FrameBg]; // sic
//
//		colors[ImGuiCol_Header] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_HeaderHovered] = colors[ImGuiCol_FrameBgHovered];
//		colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_FrameBgActive];
//
//		colors[ImGuiCol_SeparatorHovered] = colors[ImGuiCol_Separator];
//		colors[ImGuiCol_SeparatorActive] = colors[ImGuiCol_Separator];
//
//		colors[ImGuiCol_ResizeGrip] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_ResizeGripHovered] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_ResizeGripActive] = colors[ImGuiCol_FrameBg];
//
//		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_FrameBg];
//		colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg];
	}
	
	void SaveStyleColors(YAML::Node node) {
		for (int i = 0; i < ImGuiCol_COUNT; ++i) {
			node[i] = mtl::float4(ImGui::GetStyle().Colors[i]);
		}
	}

	void LoadStyleColors(YAML::Node node) {
		try {
			for (int i = 0; i < ImGuiCol_COUNT; ++i) {
				ImGui::GetStyle().Colors[i] = node[i].as<mtl::float4>();
			}
		}
		catch (...) {
            Logger::warn("Failed to load Style Colors");
		}
	}
#endif // 0

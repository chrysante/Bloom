#pragma once

#include <imgui/imgui.h>
#include <mtl/mtl.hpp>
#include <utl/vector.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Core/Serialize.hpp"

namespace poppy {

enum struct AppearanceType { none = -1, light, dark };

struct StyleVars {
    bool windowBorder;
    bool childWindowBorder;
    bool popupBorder;
    bool frameBorder;
    bool tabBorder;

    float windowRounding;
    float childWindowRounding;
    float popupRounding;
    float frameRounding;
    float tabRounding;
    float selectableRounding;

    float grabMinSize;
    float grabRounding;

    float scrollbarSize;
    float scrollbarRounding;

    float windowTitleAlign = 0.5;
};

struct StyleColorDescription {
    mtl::float4 controlFrame;
    mtl::float4 controlFrameHovered;
    mtl::float4 controlFrameDown;

    mtl::float4 highlightControlFrame;
    mtl::float4 highlightControlFrameHovered;
    mtl::float4 highlightControlFrameDown;

    mtl::float4 checkMark;

    mtl::float4 tab;
    mtl::float4 tabActive;
    mtl::float4 tabUnfocused;
    mtl::float4 tabUnfocusedActive;

    mtl::float4 text = { 0, 0, 0, 1 };
    mtl::float4 textDisabled;
    mtl::float4 windowBG = { 0, 0, 1, 1 };
    mtl::float4 popupBG;
    mtl::float4 menubarBG;

    mtl::float4 border;

    mtl::float4 windowTitleBar;
    mtl::float4 activeWindowTitleBar;

    mtl::float4 separator;
    mtl::float4 scrollbar;
    mtl::float4 dockPreview;
};

struct StyleColors {
    mtl::float4 text;
    mtl::float4 textDisabled;
    mtl::float4 windowBG;
    mtl::float4 childWindowBG;
    mtl::float4 popupBG;
    mtl::float4 menubarBG;

    mtl::float4 controlFrame;
    mtl::float4 controlFrameHovered;
    mtl::float4 controlFrameDown;

    mtl::float4 highlightControlFrame;
    mtl::float4 highlightControlFrameHovered;
    mtl::float4 highlightControlFrameDown;

    mtl::float4 checkMark;

    mtl::float4 windowTitleBar;
    mtl::float4 activeWindowTitleBar;

    mtl::float4 border;
    mtl::float4 scrollbar;

    mtl::float4 separator;

    mtl::float4 tab;
    mtl::float4 tabActive;
    mtl::float4 tabUnfocused;
    mtl::float4 tabUnfocusedActive;

    mtl::float4 dockingPreview;
};

struct Style {
    StyleVars vars;
    StyleColors colors;
};

StyleColors makeStyle(float hue, StyleColorDescription const&);

class Appearance {
public:
    Appearance();
    void update(bool force = false);

    void deserialize(YAML::Node);
    YAML::Node serialize() const;

    void showInspector(bool* open = nullptr);

    AppearanceType type() const { return mType; }

    Style const& style() const { return mStyle; }

private:
    AppearanceType mType = AppearanceType::none;

    float globalHue = 0;
    StyleColorDescription lightColorDesc;
    StyleColorDescription darkColorDesc;

    Style mStyle;
};

extern Appearance appearance;

void systemStyleInspector(bool* open = nullptr);

} // namespace poppy

BLOOM_MAKE_TEXT_SERIALIZER(poppy::StyleVars,
                           windowBorder,
                           childWindowBorder,
                           popupBorder,
                           frameBorder,
                           tabBorder,
                           windowRounding,
                           childWindowRounding,
                           popupRounding,
                           frameRounding,
                           tabRounding,
                           selectableRounding,
                           grabMinSize,
                           grabRounding,
                           scrollbarSize,
                           scrollbarRounding,
                           windowTitleAlign);

BLOOM_MAKE_TEXT_SERIALIZER(poppy::StyleColorDescription,
                           controlFrame,
                           controlFrameHovered,
                           controlFrameDown,
                           highlightControlFrame,
                           highlightControlFrameHovered,
                           highlightControlFrameDown,
                           checkMark,
                           tab,
                           tabActive,
                           tabUnfocused,
                           tabUnfocusedActive,
                           text,
                           textDisabled,
                           windowBG,
                           popupBG,
                           menubarBG,
                           border,
                           windowTitleBar,
                           activeWindowTitleBar,
                           separator,
                           scrollbar,
                           dockPreview);

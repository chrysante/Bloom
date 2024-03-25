#ifndef POPPY_UI_APPEARANCE_H
#define POPPY_UI_APPEARANCE_H

#include <imgui.h>
#include <utl/vector.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

namespace poppy {

///
enum struct AppearanceType { None = -1, Light, Dark };

/// UI variables used in the application
struct StyleVars {
    bool windowBorder;
    bool childWindowBorder;
    bool popupBorder;
    bool frameBorder;
    bool tabBorder;
    bool tabBarBorder;
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
    vml::float2 windowTitleAlign;
};

/// UI colors used in the application
struct StyleColors {
    vml::float4 text;
    vml::float4 textDisabled;
    vml::float4 windowBG;
    vml::float4 childWindowBG;
    vml::float4 popupBG;
    vml::float4 menubarBG;
    vml::float4 controlFrame;
    vml::float4 controlFrameHovered;
    vml::float4 controlFrameDown;
    vml::float4 highlightControlFrame;
    vml::float4 highlightControlFrameHovered;
    vml::float4 highlightControlFrameDown;
    vml::float4 checkMark;
    vml::float4 windowTitleBar;
    vml::float4 activeWindowTitleBar;
    vml::float4 border;
    vml::float4 scrollbar;
    vml::float4 separator;
    vml::float4 tab;
    vml::float4 tabHovered;
    vml::float4 tabActive;
    vml::float4 tabUnfocused;
    vml::float4 tabUnfocusedActive;
    vml::float4 dockingPreview;
};

/// Intermediate data structure that is derived from system colors or hard coded
/// and then used to set ImGui colors and style vars
struct StyleDescription {
    StyleColors colors;
    StyleVars vars;
};

/// Singleton describing color and style variables of the application
class Appearance {
public:
    /// Debug tool to inspect and temporarily edit the appearance
    static void showInspector(bool* open = nullptr);

    /// Light or dark
    AppearanceType type() const { return mType; }

    /// The current style
    StyleDescription const& style() const { return mStyle; }

    /// \Returns the global instance
    ///
    /// Only the editor has mutable access to the instance
    static Appearance const& get() { return staticInstance(); }

private:
    Appearance();

    friend class Editor;

    void update(bool force = false);

    static Appearance& staticInstance();

    AppearanceType mType = AppearanceType::None;
    float globalHue = 0;
    StyleDescription mStyle;
};

void systemStyleInspector(bool* open = nullptr);

} // namespace poppy

#endif // POPPY_UI_APPEARANCE_H

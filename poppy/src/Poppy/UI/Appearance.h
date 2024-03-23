#ifndef POPPY_UI_APPEARANCE_H
#define POPPY_UI_APPEARANCE_H

#include <imgui.h>
#include <mtl/mtl.hpp>
#include <utl/vector.hpp>
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
    mtl::float2 windowTitleAlign;
};

/// UI colors used in the application
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
    mtl::float4 tabHovered;
    mtl::float4 tabActive;
    mtl::float4 tabUnfocused;
    mtl::float4 tabUnfocusedActive;
    mtl::float4 dockingPreview;
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

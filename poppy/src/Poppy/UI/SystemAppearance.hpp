#pragma once

#include <array>

#include <imgui/imgui.h>
#include <mtl/mtl.hpp>

#include "Poppy/UI/Appearance.hpp"

namespace poppy {

struct SystemAppearance {
    AppearanceType type;

    enum struct ControlTint { Default = 0, Blue = 1, Graphite = 6, Clear = 7 };

    static SystemAppearance getCurrent();
    static SystemAppearance get(AppearanceType);
    static AppearanceType getCurrentType();

    /// MARK: Label Colors
    /// The primary color to use for text labels.
    mtl::double4 labelColor;

    /// The secondary color to use for text labels.
    mtl::double4 secondaryLabelColor;

    /// The tertiary color to use for text labels.
    mtl::double4 tertiaryLabelColor;

    /// The quaternary color to use for text labels and separators.
    mtl::double4 quaternaryLabelColor;

    /// MARK: Text Colors
    /// The color to use for text.
    mtl::double4 textColor;

    /// The color to use for placeholder text in controls or text views.
    mtl::double4 placeholderTextColor;

    /// The color to use for selected text.
    mtl::double4 selectedTextColor;

    /// The color to use for the background area behind text.
    mtl::double4 textBackgroundColor;

    /// The color to use for the background of selected text.
    mtl::double4 selectedTextBackgroundColor;

    /// The color to use for the keyboard focus ring around controls.
    mtl::double4 keyboardFocusIndicatorColor;

    /// The color to use for selected text in an unemphasized context.
    mtl::double4 unemphasizedSelectedTextColor;

    /// The color to use for the text background in an unemphasized context.
    mtl::double4 unemphasizedSelectedTextBackgroundColor;

    /// MARK: Content Colors
    /// The color to use for links.
    mtl::double4 linkColor;

    /// The color to use for separators between different sections of content.
    mtl::double4 separatorColor;

    /// The color to use for the background of selected and emphasized content.
    mtl::double4 selectedContentBackgroundColor;

    /// The color to use for selected and unemphasized content.
    mtl::double4 unemphasizedSelectedContentBackgroundColor;

    /// Menu Colors
    /// The color to use for the text in menu items.
    mtl::double4 selectedMenuItemTextColor;

    /// MARK: Table Colors
    /// The color to use for the optional gridlines, such as those in a table
    /// view.
    mtl::double4 gridColor;

    /// The color to use for text in header cells in table views and outline
    /// views.
    mtl::double4 headerTextColor;

    /// The colors to use for alternating content, typically found in table
    /// views and collection views.
    std::array<mtl::double4, 2> alternatingContentBackgroundColors;

    /// MARK: Control Colors
    /// The user's current accent color preference.
    mtl::double4 controlAccentColor;

    /// The color to use for the flat surfaces of a control.
    mtl::double4 controlColor;

    /// The color to use for the background of large controls, such as scroll
    /// views or table views.
    mtl::double4 controlBackgroundColor;

    /// The color to use for text on enabled controls.
    mtl::double4 controlTextColor;

    /// The color to use for text on disabled controls.
    mtl::double4 disabledControlTextColor;

    /// The current system control tint color.
    ControlTint currentControlTint;
    /// The color to use for the face of a selected control—that is, a control
    /// that has been clicked or is being dragged.
    mtl::double4 selectedControlColor;

    /// The color to use for text in a selected control—that is, a control being
    /// clicked or dragged.
    mtl::double4 selectedControlTextColor;

    /// The color to use for text in a selected control.
    mtl::double4 alternateSelectedControlTextColor;

    /// The patterned color to use for the background of a scrubber control.
    mtl::double4 scrubberTexturedBackground;

    /// MARK: Window Colors
    /// The color to use for the window background.
    mtl::double4 windowBackgroundColor;

    /// The color to use for text in a window's frame.
    mtl::double4 windowFrameTextColor;

    /// The color to use in the area beneath your window's views.
    mtl::double4 underPageBackgroundColor;

    /// MARK: Highlights and Shadows
    /// The highlight color to use for the bubble that shows inline search
    /// result values.
    mtl::double4 findHighlightColor;

    /// The color to use as a virtual light source on the screen.
    mtl::double4 highlightColor;

    /// The color to use for virtual shadows cast by raised objects on the
    /// screen.
    mtl::double4 shadowColor;

    ///
    std::array<mtl::double4, 6> logTextColors;
};

} // namespace poppy

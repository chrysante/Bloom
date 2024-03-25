#ifndef POPPY_UI_SYSTEMAPPEARANCE_H
#define POPPY_UI_SYSTEMAPPEARANCE_H

#include <array>

#include <imgui.h>
#include <vml/vml.hpp>

#include "Poppy/UI/Appearance.h"

namespace poppy {

struct SystemAppearance {
    AppearanceType type;

    enum class ControlTint { Default = 0, Blue = 1, Graphite = 6, Clear = 7 };

    static SystemAppearance getCurrent();
    static SystemAppearance get(AppearanceType);
    static AppearanceType getCurrentType();

    /// The primary color to use for text labels.
    vml::double4 labelColor;

    /// The secondary color to use for text labels.
    vml::double4 secondaryLabelColor;

    /// The tertiary color to use for text labels.
    vml::double4 tertiaryLabelColor;

    /// The quaternary color to use for text labels and separators.
    vml::double4 quaternaryLabelColor;

    /// The color to use for text.
    vml::double4 textColor;

    /// The color to use for placeholder text in controls or text views.
    vml::double4 placeholderTextColor;

    /// The color to use for selected text.
    vml::double4 selectedTextColor;

    /// The color to use for the background area behind text.
    vml::double4 textBackgroundColor;

    /// The color to use for the background of selected text.
    vml::double4 selectedTextBackgroundColor;

    /// The color to use for the keyboard focus ring around controls.
    vml::double4 keyboardFocusIndicatorColor;

    /// The color to use for selected text in an unemphasized context.
    vml::double4 unemphasizedSelectedTextColor;

    /// The color to use for the text background in an unemphasized context.
    vml::double4 unemphasizedSelectedTextBackgroundColor;

    /// MARK: Content Colors
    /// The color to use for links.
    vml::double4 linkColor;

    /// The color to use for separators between different sections of content.
    vml::double4 separatorColor;

    /// The color to use for the background of selected and emphasized content.
    vml::double4 selectedContentBackgroundColor;

    /// The color to use for selected and unemphasized content.
    vml::double4 unemphasizedSelectedContentBackgroundColor;

    /// Menu Colors
    /// The color to use for the text in menu items.
    vml::double4 selectedMenuItemTextColor;

    /// MARK: Table Colors
    /// The color to use for the optional gridlines, such as those in a table
    /// view.
    vml::double4 gridColor;

    /// The color to use for text in header cells in table views and outline
    /// views.
    vml::double4 headerTextColor;

    /// The colors to use for alternating content, typically found in table
    /// views and collection views.
    std::array<vml::double4, 2> alternatingContentBackgroundColors;

    /// MARK: Control Colors
    /// The user's current accent color preference.
    vml::double4 controlAccentColor;

    /// The color to use for the flat surfaces of a control.
    vml::double4 controlColor;

    /// The color to use for the background of large controls, such as scroll
    /// views or table views.
    vml::double4 controlBackgroundColor;

    /// The color to use for text on enabled controls.
    vml::double4 controlTextColor;

    /// The color to use for text on disabled controls.
    vml::double4 disabledControlTextColor;

    /// The current system control tint color.
    ControlTint currentControlTint;
    /// The color to use for the face of a selected control—that is, a control
    /// that has been clicked or is being dragged.
    vml::double4 selectedControlColor;

    /// The color to use for text in a selected control—that is, a control being
    /// clicked or dragged.
    vml::double4 selectedControlTextColor;

    /// The color to use for text in a selected control.
    vml::double4 alternateSelectedControlTextColor;

    /// The patterned color to use for the background of a scrubber control.
    vml::double4 scrubberTexturedBackground;

    /// MARK: Window Colors
    /// The color to use for the window background.
    vml::double4 windowBackgroundColor;

    /// The color to use for text in a window's frame.
    vml::double4 windowFrameTextColor;

    /// The color to use in the area beneath your window's views.
    vml::double4 underPageBackgroundColor;

    /// MARK: Highlights and Shadows
    /// The highlight color to use for the bubble that shows inline search
    /// result values.
    vml::double4 findHighlightColor;

    /// The color to use as a virtual light source on the screen.
    vml::double4 highlightColor;

    /// The color to use for virtual shadows cast by raised objects on the
    /// screen.
    vml::double4 shadowColor;

    ///
    std::array<vml::double4, 6> logTextColors;
};

} // namespace poppy

#endif // POPPY_UI_SYSTEMAPPEARANCE_H

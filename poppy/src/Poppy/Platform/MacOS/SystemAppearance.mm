#include "Poppy/UI/SystemAppearance.hpp"

#import <Appkit/NSColor.h>

#include "Poppy/Core/Debug.hpp"

using namespace poppy;
	
static mtl::double4 convertColor(NSColor* color) {
    CIColor* tmp = [[CIColor alloc] initWithColor: color];
    return {
        tmp.red,
        tmp.green,
        tmp.blue,
        tmp.alpha
    };
    return {};
}

AppearanceType SystemAppearance::getCurrentType() {
    auto appearance = NSApp.effectiveAppearance;
    
    if (appearance.name == NSAppearanceNameAqua) {
        return AppearanceType::light;
    }
    else if (appearance.name == NSAppearanceNameDarkAqua) {
        return AppearanceType::dark;
    }
    else {
        return AppearanceType::none;
    }
}

static NSAppearanceName toNSType(AppearanceType type) {
    switch (type) {
        case AppearanceType::light:
            return NSAppearanceNameAqua;
        case AppearanceType::dark:
            return NSAppearanceNameDarkAqua;
        default:
            BL_DEBUGFAIL();
    }
}

SystemAppearance SystemAppearance::getCurrent() {
    return get(getCurrentType());
}

SystemAppearance SystemAppearance::get(AppearanceType type) {
    __block SystemAppearance result{};
    result.type = SystemAppearance::getCurrentType();
    [[NSAppearance appearanceNamed:toNSType(type)] performAsCurrentDrawingAppearance:^(){
        result.labelColor                                 = convertColor(NSColor.labelColor);
        result.secondaryLabelColor                        = convertColor(NSColor.secondaryLabelColor);
        result.tertiaryLabelColor                         = convertColor(NSColor.tertiaryLabelColor);
        result.quaternaryLabelColor                       = convertColor(NSColor.quaternaryLabelColor);
        result.textColor                                  = convertColor(NSColor.textColor);
        result.placeholderTextColor                       = convertColor(NSColor.placeholderTextColor);
        result.selectedTextColor                          = convertColor(NSColor.selectedTextColor);
        result.textBackgroundColor                        = convertColor(NSColor.textBackgroundColor);
        result.selectedTextBackgroundColor                = convertColor(NSColor.selectedTextBackgroundColor);
        result.keyboardFocusIndicatorColor                = convertColor(NSColor.keyboardFocusIndicatorColor);
        result.unemphasizedSelectedTextColor              = convertColor(NSColor.unemphasizedSelectedTextColor);
        result.unemphasizedSelectedTextBackgroundColor    = convertColor(NSColor.unemphasizedSelectedTextBackgroundColor);
        result.linkColor                                  = convertColor(NSColor.linkColor);
        result.separatorColor                             = convertColor(NSColor.separatorColor);
        result.selectedContentBackgroundColor             = convertColor(NSColor.selectedContentBackgroundColor);
        result.unemphasizedSelectedContentBackgroundColor = convertColor(NSColor.unemphasizedSelectedContentBackgroundColor);
        result.selectedMenuItemTextColor                  = convertColor(NSColor.selectedMenuItemTextColor);
        result.gridColor                                  = convertColor(NSColor.gridColor);
        result.headerTextColor                            = convertColor(NSColor.headerTextColor);
        result.alternatingContentBackgroundColors[0]      = convertColor(NSColor.alternatingContentBackgroundColors[0]);
        result.alternatingContentBackgroundColors[1]      = convertColor(NSColor.alternatingContentBackgroundColors[1]);
        result.controlAccentColor                         = convertColor(NSColor.controlAccentColor);
        result.controlColor                               = convertColor(NSColor.controlColor);
        result.controlBackgroundColor                     = convertColor(NSColor.controlBackgroundColor);
        result.controlTextColor                           = convertColor(NSColor.controlTextColor);
        result.disabledControlTextColor                   = convertColor(NSColor.disabledControlTextColor);
        result.currentControlTint                         = (ControlTint)NSColor.currentControlTint;
        result.selectedControlColor                       = convertColor(NSColor.selectedControlColor);
        result.selectedControlTextColor                   = convertColor(NSColor.selectedControlTextColor);
        result.alternateSelectedControlTextColor          = convertColor(NSColor.alternateSelectedControlTextColor);
        result.scrubberTexturedBackground                 = convertColor(NSColor.scrubberTexturedBackgroundColor);
        result.windowBackgroundColor                      = convertColor(NSColor.windowBackgroundColor);
        result.windowFrameTextColor                       = convertColor(NSColor.windowFrameTextColor);
        result.underPageBackgroundColor                   = convertColor(NSColor.underPageBackgroundColor);
        result.findHighlightColor                         = convertColor(NSColor.findHighlightColor);
        result.highlightColor                             = convertColor(NSColor.highlightColor);
        result.shadowColor                                = convertColor(NSColor.shadowColor);
    }];
    return result;
}

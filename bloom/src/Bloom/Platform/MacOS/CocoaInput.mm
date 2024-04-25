#include "Bloom/Core/Base.h"

#include <AppKit/AppKit.h>
#include <utl/hashtable.hpp>
#include <utl/utility.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/InputInternal.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Platform/MacOS/CocoaImpl.h"

using namespace bloom;
using namespace vml::short_types;

static Key translateKeyCodeGerman(unsigned code) {
    using enum Key;
    switch (code) {
    case 53:
        return Key::Escape;
    case 122:
        return Key::F1;
    case 120:
        return Key::F2;
    case 99:
        return Key::F3;
    case 118:
        return Key::F4;
    case 96:
        return Key::F5;
    case 98:
        return Key::F7;
    case 100:
        return Key::F8;
    case 101:
        return Key::F9;
    case 109:
        return Key::F10;
    case 103:
        return Key::F11;
    case 111:
        return Key::F12;
    case 10:
        return Key::Circumflex;
    case 18:
        return Key::Num1;
    case 19:
        return Key::Num2;
    case 20:
        return Key::Num3;
    case 21:
        return Key::Num4;
    case 23:
        return Key::Num5;
    case 22:
        return Key::Num6;
    case 26:
        return Key::Num7;
    case 28:
        return Key::Num8;
    case 25:
        return Key::Num9;
    case 29:
        return Key::Num0;
    case 24:
        return Key::Accent;
    case 51:
        return Key::Backspace;
    case 48:
        return Key::Tab;
    case 12:
        return Key::Q;
    case 13:
        return Key::W;
    case 14:
        return Key::E;
    case 15:
        return Key::R;
    case 17:
        return Key::T;
    case 16:
        return Key::Z;
    case 32:
        return Key::U;
    case 34:
        return Key::I;
    case 31:
        return Key::O;
    case 35:
        return Key::P;
    case 30:
        return Key::Plus;
    case 36:
        return Key::Enter;
    case 0:
        return Key::A;
    case 1:
        return Key::S;
    case 2:
        return Key::D;
    case 3:
        return Key::F;
    case 5:
        return Key::G;
    case 4:
        return Key::H;
    case 38:
        return Key::J;
    case 40:
        return Key::K;
    case 37:
        return Key::L;
    case 42:
        return Key::Hash;
    case 50:
        return Key::AngleBracketOpen;
    case 6:
        return Key::Y;
    case 7:
        return Key::X;
    case 8:
        return Key::C;
    case 9:
        return Key::V;
    case 11:
        return Key::B;
    case 45:
        return Key::N;
    case 46:
        return Key::M;
    case 43:
        return Key::Comma;
    case 47:
        return Key::Period;
    case 44:
        return Key::Minus;
    case 49:
        return Key::Space;
    case 126:
        return Key::UpArrow;
    case 125:
        return Key::DownArrow;
    case 123:
        return Key::LeftArrow;
    case 124:
        return Key::RightArrow;
    default:
        return Key::None;
    }
}

static MouseButton translateMouseButton(NSEventType type) {
    switch (type) {
    case NSEventTypeLeftMouseDown:
        [[fallthrough]];
    case NSEventTypeLeftMouseUp:
        return MouseButton::Left;
    case NSEventTypeRightMouseDown:
        [[fallthrough]];
    case NSEventTypeRightMouseUp:
        return MouseButton::Right;
    case NSEventTypeOtherMouseDown:
        [[fallthrough]];
    case NSEventTypeOtherMouseUp:
        return MouseButton::Other;
    default:
        return MouseButton::None;
    }
}

static bool isMouseButtonDown(NSEventType type) {
    switch (type) {
    case NSEventTypeLeftMouseUp:
        [[fallthrough]];
    case NSEventTypeRightMouseUp:
        [[fallthrough]];
    case NSEventTypeOtherMouseUp: {
        return false;
    }
    case NSEventTypeLeftMouseDown:
        [[fallthrough]];
    case NSEventTypeRightMouseDown:
        [[fallthrough]];
    case NSEventTypeOtherMouseDown: {
        return true;
    }
    default:
        BL_UNREACHABLE();
    }
}

static ModFlags translateModFlags(NSEventModifierFlags flags) {
    using enum ModFlags;
    ModFlags result = None;
    if (flags & NSEventModifierFlagCapsLock) {
        result |= CapsLock;
    }
    if (flags & NSEventModifierFlagShift) {
        result |= Shift;
    }
    if (flags & NSEventModifierFlagControl) {
        result |= Control;
    }
    if (flags & NSEventModifierFlagOption) {
        result |= Alt;
    }
    if (flags & NSEventModifierFlagCommand) {
        result |= Super;
    }
    if (flags & NSEventModifierFlagNumericPad) {
        result |= NumLock;
    }
    return result;
}

static InputEventMask translateType(NSEventType type) {
    switch (type) {
    case NSEventTypeLeftMouseDown:
        return InputEventMask::LeftMouseDown;
    case NSEventTypeLeftMouseUp:
        return InputEventMask::LeftMouseUp;
    case NSEventTypeRightMouseDown:
        return InputEventMask::RightMouseDown;
    case NSEventTypeRightMouseUp:
        return InputEventMask::RightMouseUp;
    case NSEventTypeOtherMouseDown:
        return InputEventMask::OtherMouseDown;
    case NSEventTypeOtherMouseUp:
        return InputEventMask::OtherMouseUp;
    case NSEventTypeKeyDown:
        return InputEventMask::KeyDown;
    case NSEventTypeKeyUp:
        return InputEventMask::KeyUp;
    default:
        return InputEventMask::None;
    }
}

static double2 getLocationInWindow(NSEvent* event) {
    double2 p = event.locationInWindow;
    p.y = event.window.frame.size.height - p.y;
    return p;
}

static double2 getMouseLocationOnScreen() {
    double2 p = NSEvent.mouseLocation;
    p.y = CGDisplayPixelsHigh(kCGDirectMainDisplay) - p.y;
    return p;
}

static MouseEvent makeMouseEvent(NSEvent* event) {
    MouseEvent result{};
    result.modifierFlags = translateModFlags(event.modifierFlags);
    result.locationInWindow = getLocationInWindow(event);
    result.locationOnScreen = getMouseLocationOnScreen();
    return result;
}

static InputEvent makeMouseButtonEvent(NSEvent* event) {
    MouseEvent base = makeMouseEvent(event);
    auto type = translateType(event.type);
    MouseButton button = translateMouseButton(event.type);
    if (isMouseButtonDown(event.type)) {
        MouseDownEvent downEvent{ base, .button = button,
                                  .clickCount = event.clickCount };
        return InputEvent(type, downEvent);
    }
    else {
        MouseUpEvent upEvent{ base, .button = button };
        return InputEvent(type, upEvent);
    }
}

static InputEvent makeScrollEvent(NSEvent* event) {
    ScrollEvent result{ makeMouseEvent(event),
                        .offset = { event.scrollingDeltaX,
                                    event.scrollingDeltaY },
                        .isTrackpad = event.momentumPhase != NSEventPhaseNone ||
                                      event.phase != NSEventPhaseNone };
    return InputEvent(InputEventMask::ScrollWheel, result);
}

static InputEvent makeMouseMoveDragEvent(NSEvent* event) {
    MouseMoveEvent base{ makeMouseEvent(event),
                         .offset = { event.deltaX, event.deltaY } };
    switch (event.type) {
    case NSEventTypeMouseMoved:
        return InputEvent(InputEventMask::MouseMoved, base);
    case NSEventTypeLeftMouseDragged:
        return InputEvent(InputEventMask::LeftMouseDragged,
                          MouseDragEvent{ base, .button = MouseButton::Left });
    case NSEventTypeRightMouseDragged:
        return InputEvent(InputEventMask::RightMouseDragged,
                          MouseDragEvent{ base, .button = MouseButton::Right });
    case NSEventTypeOtherMouseDragged:
        return InputEvent(InputEventMask::OtherMouseDragged,
                          MouseDragEvent{ base, .button = MouseButton::Other });
    default:
        BL_UNREACHABLE(); // Invalid cocoa event
    }
}

static InputEvent makeMagnificationEvent(NSEvent* event) {
    MagnificationEvent result{ makeMouseEvent(event),
                               .offset = event.magnification };
    return InputEvent(InputEventMask::Magnify, result);
}

static InputEvent makeKeyEvent(NSEvent* event) {
    KeyEvent keyEvent{ { .modifierFlags =
                             translateModFlags(event.modifierFlags) },
                       .key = translateKeyCodeGerman(event.keyCode) };
    return InputEvent(translateType(event.type), keyEvent);
}

namespace {

using InputCallback = std::function<void(InputEvent const&, void*)>;

class CallbackMap {
public:
    int add(InputCallback callback) {
        static int ID = 0;
        bool result = map.insert({ ID, std::move(callback) }).second;
        BL_ASSERT(result, "Failed to register callback");
        return ID++;
    }

    void remove(int ID) {
        bool erased = map.erase(ID);
        BL_ASSERT(
            erased,
            "Failed to unregister callback. Has this callback been unregistered before?");
    }

    void invokeAll(InputEvent const& event, NSEvent* native) const {
        for (auto& [ID, callback]: map) {
            callback(event, (__bridge void*)native);
        }
    }

private:
    utl::hashmap<int, InputCallback> map;
};

CallbackMap gCallbackMap;

} // namespace

static void createGlobalMonitors();

int bloom::registerGlobalInputCallback(
    std::function<void(InputEvent const&, void*)> handler) {
    [[maybe_unused]] static int const _ = [] {
        createGlobalMonitors();
        return 0;
    }();
    return gCallbackMap.add(std::move(handler));
}

void bloom::unregisterGlobalInputCallback(int ID) { gCallbackMap.remove(ID); }

/// \Returns `true` if the location of \p event is over the frame of \p button
static bool isOverWindowButton(NSEvent* event, NSWindowButton button) {
    auto buttonFrame = [event.window standardWindowButton:button].frame;
    auto windowFrame = event.window.frame;
    buttonFrame.origin.y = windowFrame.size.height - buttonFrame.origin.y -
                           buttonFrame.size.height;
    return NSPointInRect(event.locationInWindow, buttonFrame);
}

/// \Returns `true` if \p value is in the interval `[min + padding, max -
/// padding]`
static bool isInBounds(double value, double min, double max, double padding) {
    return value >= min + padding && value <= max - padding;
}

/// \Returns `true` if the location of \p event is within the area of the window
/// where the window is resizeable, i.e. in any window location where the mouse
/// image changes to a resize indicator. This is used to avoid starting window
/// drag operations when resizing
static bool isInResizeArea(NSEvent* event) {
    // Padding     Corner Padding
    // vvv         vvvvv
    // +---------------+ -<
    // |   +-------+   | |< Corner Padding
    // |   |       |   | |<
    // | +-+       +-+ | -<
    // | |           | |
    // | |           | |
    // | +-+       +-+ |
    // |   |       |   |
    // |   +-------+   | -< Padding
    // +---------------+ -<
    //
    // The set difference of the shapes above is the resize area. The padding
    // values used below are determined by experimentation
    auto frame = event.window.frame;
    auto location = event.locationInWindow;
    // Borders
    double padding = 3;
    if (!isInBounds(location.x, 0, frame.size.width, padding) ||
        !isInBounds(location.y, 0, frame.size.height, padding))
    {
        return true;
    }
    // Corners
    double cornerPadding = 12;
    if (!isInBounds(location.x, 0, frame.size.width, cornerPadding) &&
        !isInBounds(location.y, 0, frame.size.height, cornerPadding))
    {
        return true;
    }
    return false;
}

/// \Returns `true` if \p event may be used to start a window drag operation
/// without interfering with the window buttons and resize operations
static bool isResizeOrWindowButtonInteraction(NSEvent* event) {
    return isInResizeArea(event) ||
           isOverWindowButton(event, NSWindowCloseButton) ||
           isOverWindowButton(event, NSWindowMiniaturizeButton) ||
           isOverWindowButton(event, NSWindowZoomButton);
}

static NSEvent* handleEvent(NSEvent* event) {
    switch (event.type) {
    case NSEventTypeLeftMouseDown:
        [[fallthrough]];
    case NSEventTypeLeftMouseUp:
        [[fallthrough]];
    case NSEventTypeRightMouseDown:
        [[fallthrough]];
    case NSEventTypeRightMouseUp:
        [[fallthrough]];
    case NSEventTypeOtherMouseDown:
        [[fallthrough]];
    case NSEventTypeOtherMouseUp:
        /// Any events that interact with
        /// window resizing or the window
        /// buttons are not forwarded to
        /// the application
        if (!isResizeOrWindowButtonInteraction(event) ||
            !isMouseButtonDown(event.type))
        {
            gCallbackMap.invokeAll(makeMouseButtonEvent(event), event);
        }
        break;
    case NSEventTypeScrollWheel:
        gCallbackMap.invokeAll(makeScrollEvent(event), event);
        break;
    case NSEventTypeMouseMoved:
        [[fallthrough]];
    case NSEventTypeLeftMouseDragged:
        [[fallthrough]];
    case NSEventTypeRightMouseDragged:
        [[fallthrough]];
    case NSEventTypeOtherMouseDragged:
        gCallbackMap.invokeAll(makeMouseMoveDragEvent(event), event);
        break;
    case NSEventTypeMagnify:
        gCallbackMap.invokeAll(makeMagnificationEvent(event), event);
        break;
    case NSEventTypeKeyUp:
        [[fallthrough]];
    case NSEventTypeKeyDown:
        gCallbackMap.invokeAll(makeKeyEvent(event), event);
        break;
    case NSEventTypeApplicationDefined:
        if ((CocoaEventType)event.subtype == CocoaEventType::WindowShallZoom) {
            [event.window zoom:nil];
        }
        break;
    default:
        break;
    }
    return event;
}

static void createGlobalMonitors() {
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskAny
                                          handler:^(NSEvent* event) {
                                              return handleEvent(event);
                                          }];
}

// We implement this here so we don't have to add a CocoaApplication.mm file for
// this one function
void Application::doOnInput(InputEvent const& event, void* native) {
    inputUpdate(mInput, event);
    onInput(event);
    NSEvent* nsEvent = (__bridge NSEvent*)native;
    for (auto& window: mWindows) {
        /// This could be called from `destroyWindow()` by GLFW as a last step
        /// before destroying the window, i.e. where the window pointer here is
        /// already null
        if (!window) {
            continue;
        }
        if ((__bridge NSWindow*)window->nativeHandle() != nsEvent.window) {
            continue;
        }
        window->onInputImpl(event, native);
        break;
    }
}

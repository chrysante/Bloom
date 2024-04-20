#include "Bloom/Core/Base.h"

#define GLFW_EXPOSE_NATIVE_COCOA

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <utl/utility.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"

using namespace bloom;
using namespace vml;

static NSWindow* getNative(Window const* window) {
    return (__bridge NSWindow*)const_cast<Window*>(window)->nativeHandle();
}

void* Window::nativeHandle() {
    return (__bridge void*)glfwGetCocoaWindow((GLFWwindow*)windowPtr.get());
}

void Window::setSwapchain(std::unique_ptr<Swapchain> swapchain) {
    _swapchain = std::move(swapchain);
    MetalSwapchain& mtlSwapchain =
        dynamic_cast<MetalSwapchain&>(this->swapchain());
    NSWindow* window = getNative(this);
    window.contentView.wantsLayer = true;
    [window.contentView setLayer:mtlSwapchain.layer];
}

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

static MouseEvent makeMouseEvent(NSEvent* event) {
    MouseEvent result{};
    result.modifierFlags = translateModFlags(event.modifierFlags);
    result.locationInWindow = event.locationInWindow;
    result.locationOnScreen = NSEvent.mouseLocation;
    result.locationOnScreen.y =
        CGDisplayPixelsHigh(kCGDirectMainDisplay) - result.locationOnScreen.y;
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
    MagnificationEvent result{};
    result.modifierFlags = translateModFlags(event.modifierFlags);
    result.locationInWindow = event.locationInWindow;
    result.offset = event.magnification;
    return InputEvent(InputEventMask::Magnify, result);
}

static InputEvent makeKeyEvent(NSEvent* event) {
    KeyEvent keyEvent{ { .modifierFlags =
                             translateModFlags(event.modifierFlags) },
                       .key = translateKeyCodeGerman(event.keyCode) };
    return InputEvent(translateType(event.type), keyEvent);
}

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
static bool mayMoveWindow(NSEvent* event) {
    return !isInResizeArea(event) &&
           !isOverWindowButton(event, NSWindowCloseButton) &&
           !isOverWindowButton(event, NSWindowMiniaturizeButton) &&
           !isOverWindowButton(event, NSWindowZoomButton);
}

namespace {

/// Our event types that we send through cocoa
enum class CocoaEventType : short { WindowShallZoom };

} // namespace

void Window::platformInit() {
    NSWindow* window = getNative(this);
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
        initWithRect:NSZeroRect
             options:NSTrackingMouseMoved | NSTrackingInVisibleRect |
                     NSTrackingActiveAlways
               owner:window
            userInfo:nil];
    [window.contentView addTrackingArea:trackingArea];
    applyStyle();
    /// Because GLFW down not get mouse input events over the toolbar we monitor
    /// these events ourself
    /// TODO: Remove the monitors when we destroy the window
    auto mouseButtonHandler = ^NSEvent*(NSEvent* event) {
        if (event.window != window) {
            return event;
        }
        auto modFlags = translateModFlags(event.modifierFlags);
        auto button = translateMouseButton(event.type);
        bool isDown = isMouseButtonDown(event.type);
        userInput._modFlags = modFlags;
        /// Any events that interact with window resizing or the window buttons
        /// are not forwarded to the application
        if (!mayMoveWindow(event) && isDown) {
            return event;
        }
        userInput._mouseButtons[(size_t)button] = isDown;
        if (callbacks.onInputFn) {
            auto inputEvent = makeMouseButtonEvent(event);
            callbacks.onInputFn(inputEvent);
        }
        bool isInToolbar =
            window.frame.size.height - event.locationInWindow.y <=
            toolbarHeight();
        if (event.type == NSEventTypeLeftMouseDown && desc.movable &&
            isInToolbar)
        {
            [window performWindowDragWithEvent:event];
        }
        return event;
    };
    auto mouseButtonMask = NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp |
                           NSEventMaskRightMouseDown | NSEventMaskRightMouseUp |
                           NSEventMaskOtherMouseDown | NSEventMaskOtherMouseUp;
    [NSEvent addLocalMonitorForEventsMatchingMask:mouseButtonMask
                                          handler:mouseButtonHandler];

    auto scrollHandler = ^NSEvent*(NSEvent* event) {
        if (event.window != window) {
            return event;
        }
        userInput.setScrollOffset(
            { event.scrollingDeltaX, event.scrollingDeltaY });
        if (callbacks.onInputFn) {
            callbacks.onInputFn(makeScrollEvent(event));
        }
        return event;
    };
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel
                                          handler:scrollHandler];

    auto mouseMovedDraggedHandler = ^NSEvent*(NSEvent* event) {
        if (event.window != window) {
            return event;
        }
        if (callbacks.onInputFn) {
            callbacks.onInputFn(makeMouseMoveDragEvent(event));
        }
        return event;
    };
    auto moveDragMask = NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged |
                        NSEventMaskRightMouseDragged |
                        NSEventMaskOtherMouseDragged;
    [NSEvent addLocalMonitorForEventsMatchingMask:moveDragMask
                                          handler:mouseMovedDraggedHandler];

    auto magnifyHandler = ^NSEvent*(NSEvent* event) {
        if (event.window != window) {
            return event;
        }
        if (callbacks.onInputFn) {
            callbacks.onInputFn(makeMagnificationEvent(event));
        }
        return event;
    };
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
                                          handler:magnifyHandler];

    auto keyHandler = ^NSEvent*(NSEvent* event) {
        if (event.window != window) {
            return event;
        }
        if (callbacks.onInputFn) {
            callbacks.onInputFn(makeKeyEvent(event));
        }
        return event;
    };
    [NSEvent
        addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp | NSEventMaskKeyUp
                                     handler:keyHandler];

    auto appDefinedReceiver = ^NSEvent*(NSEvent* event) {
        if ((CocoaEventType)event.subtype == CocoaEventType::WindowShallZoom) {
            [event.window zoom:nil];
        }
        return event;
    };
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskApplicationDefined
                                          handler:appDefinedReceiver];
}

void Window::applyStyle() {
    NSWindow* window = getNative(this);
    window.title = @"";
    window.movable = false;
    window.styleMask |= NSWindowStyleMaskFullSizeContentView;
    window.titlebarAppearsTransparent = true;
    window.toolbar = [[NSToolbar alloc] init];
}

float Window::toolbarHeight() const {
    NSWindow* window = getNative(this);
    return (float)window.contentView.safeAreaInsets.top;
}

vml::AABB<float, 2> Window::titleButtonsArea() const {
    NSWindow* window = getNative(this);
    auto a = [window standardWindowButton:NSWindowCloseButton].frame;
    auto b = [window standardWindowButton:NSWindowMiniaturizeButton].frame;
    auto c = [window standardWindowButton:NSWindowZoomButton].frame;
    auto rect = NSUnionRect(NSUnionRect(a, b), c);
    return { rect.origin, { rect.size.width, rect.size.height } };
}

void Window::zoom() {
    NSWindow* window = getNative(this);
    NSEvent* event = [NSEvent
        otherEventWithType:NSEventTypeApplicationDefined
                  location:NSMakePoint(0, 0)
             modifierFlags:0
                 timestamp:0
              windowNumber:window.windowNumber
                   context:nil
                   subtype:(short)CocoaEventType::WindowShallZoom
                     data1:0
                     data2:0];
    [NSApp postEvent:event atStart:YES];
}

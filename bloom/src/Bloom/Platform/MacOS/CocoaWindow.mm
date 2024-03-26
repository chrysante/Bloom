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
    default:
        return InputEventMask::None;
    }
}

static InputEvent makeMouseButtonEvent(InputEventMask type, MouseButton button,
                                       bool isDown, int clickCount,
                                       Input const& input) {
    MouseEvent base{ { input.modFlags() }, input.mousePosition() };
    if (isDown) {
        MouseDownEvent downEvent{ base, .button = button,
                                  .clickCount = clickCount };
        return InputEvent(type, downEvent);
    }
    else {
        MouseUpEvent upEvent{ base, .button = button };
        return InputEvent(type, upEvent);
    }
}

static InputEvent makeScrollEvent(NSEvent* event) {
    ScrollEvent result{};
    result.isTrackpad = event.momentumPhase != NSEventPhaseNone ||
                        event.phase != NSEventPhaseNone;
    result.modifierFlags = translateModFlags(event.modifierFlags);
    result.locationInWindow = event.locationInWindow;
    result.offset = { event.scrollingDeltaX, event.scrollingDeltaY };
    return InputEvent(InputEventMask::ScrollWheel, result);
}

static InputEvent makeMagnificationEvent(NSEvent* event) {
    MagnificationEvent result{};
    result.modifierFlags = translateModFlags(event.modifierFlags);
    result.locationInWindow = event.locationInWindow;
    result.offset = event.magnification;
    return InputEvent(InputEventMask::Magnify, result);
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
        userInput._mouseButtons[(size_t)button] = isDown;
        userInput._modFlags = modFlags;
        if (!mayMoveWindow(event)) {
            return event;
        }
        if (callbacks.onInputFn) {
            auto inputEvent =
                makeMouseButtonEvent(translateType(event.type), button, isDown,
                                     (int)event.clickCount, userInput);
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

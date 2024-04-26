#include "Bloom/Core/Base.h"

#define GLFW_EXPOSE_NATIVE_COCOA

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <utl/utility.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Platform/MacOS/CocoaImpl.h"
#include "Bloom/Platform/MacOS/ObjCCommon.h"
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
    MetalSwapchain& mtlSwapchain = dynamic_cast<MetalSwapchain&>(*_swapchain);
    NSWindow* window = getNative(this);
    window.contentView.wantsLayer = true;
    [window.contentView setLayer:mtlSwapchain.layer];
}

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
}

void Window::applyStyle() {
    NSWindow* window = getNative(this);
    window.title = @"";
    window.movable = false;
    if (desc.noDecoration) {
        window.styleMask |= NSWindowStyleMaskBorderless;
        window.styleMask &= ~NSWindowStyleMaskTitled;
    }
    else {
        window.styleMask |= NSWindowStyleMaskFullSizeContentView;
        window.titlebarAppearsTransparent = true;
    }
    if (desc.hasToolbarArea) {
        window.toolbarStyle = NSWindowToolbarStyleUnifiedCompact;
        window.toolbar = [[NSToolbar alloc] init];
    }
}

float Window::titlebarHeight() const {
    NSWindow* window = getNative(this);
    return (float)window.contentView.safeAreaInsets.top;
}

vml::AABB<float, 2> Window::titleButtonsArea() const {
    NSWindow* window = getNative(this);
    auto a = [window standardWindowButton:NSWindowCloseButton].frame;
    auto b = [window standardWindowButton:NSWindowMiniaturizeButton].frame;
    auto c = [window standardWindowButton:NSWindowZoomButton].frame;
    auto rect = NSUnionRect(NSUnionRect(a, b), c);
    return { (float2)rect.origin,
             (float2)rect.origin + float2(rect.size.width, rect.size.height) };
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

static bool isInToolbar(Window& window, NSEvent* event) {
    return event.window.frame.size.height - event.locationInWindow.y <=
           window.titlebarHeight();
}

void Window::onInputImpl(InputEvent const& event, void* nativeEvent) {
    NSEvent* nsEvent = (__bridge NSEvent*)nativeEvent;
    auto* nsWindow = getNative(this);
    if (nsEvent.window != nsWindow) {
        return;
    }
    invokeDelegates(&WindowDelegate::onInput, event);
    if (nsEvent.type == NSEventTypeLeftMouseDown && desc.movable &&
        isInToolbar(*this, nsEvent))
    {
        switch (nsEvent.clickCount) {
        case 1:
            desc.isMoving = true;
            invokeDelegates(&WindowDelegate::onMoveStart);
            [nsWindow performWindowDragWithEvent:nsEvent];
            break;
        case 2:
            zoom();
            break;
        default:
            break;
        }
    }
    if (nsEvent.type == NSEventTypeLeftMouseUp) {
        desc.isMoving = false;
    }
}

bool Window::isMiniaturized() const { return getNative(this).isMiniaturized; }

void Window::addToolbarArea() {
    if (desc.hasToolbarArea) {
        return;
    }
    desc.hasToolbarArea = true;
    auto* nsWindow = getNative(this);
    nsWindow.toolbarStyle = NSWindowToolbarStyleUnifiedCompact;
    nsWindow.toolbar = [[NSToolbar alloc] init];
}

void Window::removeToolbarArea() {
    if (!desc.hasToolbarArea) {
        return;
    }
    desc.hasToolbarArea = false;
    auto* nsWindow = getNative(this);
    nsWindow.toolbar = nil;
}

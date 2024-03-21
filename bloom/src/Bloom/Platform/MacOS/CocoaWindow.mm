#include "Bloom/Core/Base.h"

#ifdef BLOOM_PLATFORM_APPLE
#	define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <utl/utility.hpp>
#include <mtl/mtl.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"

using namespace bloom;
using namespace mtl;

void* Window::nativeHandle() {
    return (__bridge void*)glfwGetCocoaWindow((GLFWwindow*)glfwWindowPtr.get());
}

void Window::setSwapchain(std::unique_ptr<Swapchain> swapchain) {
    _swapchain = std::move(swapchain);
    MetalSwapchain& mtlSwapchain =
        dynamic_cast<MetalSwapchain&>(this->swapchain());
    NSWindow* nsWindow = (__bridge NSWindow*)nativeHandle();
    nsWindow.contentView.wantsLayer = true;
    [nsWindow.contentView setLayer:mtlSwapchain.layer];
}

void Window::platformInit() {
    NSWindow* nsWindow = (__bridge NSWindow*)nativeHandle();
    NSTrackingArea *trackingArea =
        [[NSTrackingArea alloc] initWithRect: NSZeroRect
                                     options: NSTrackingMouseMoved |
                                              NSTrackingInVisibleRect |
                                              NSTrackingActiveAlways
                                       owner: nsWindow
                                    userInfo: nil];
    [nsWindow.contentView addTrackingArea:trackingArea];
    
    /// Code to hide the window frame. We leave this here for now
#if 0
    nsWindow.opaque = NO;
    nsWindow.styleMask =   NSWindowStyleMaskResizable
                     |   NSWindowStyleMaskTitled
                     |   NSWindowStyleMaskFullSizeContentView;
    nsWindow.movableByWindowBackground = false;
    nsWindow.titlebarAppearsTransparent = true;
    nsWindow.titleVisibility             =   NSWindowTitleHidden;
    nsWindow.showsToolbarButton          =   false;
    [nsWindow standardWindowButton:NSWindowZoomButton].hidden   =   false;
    [nsWindow standardWindowButton:NSWindowMiniaturizeButton].hidden  =   false;
    [nsWindow standardWindowButton:NSWindowCloseButton].hidden        =   false;
    [nsWindow standardWindowButton:NSWindowZoomButton].hidden         =   false;
#endif
}

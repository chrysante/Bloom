#include "Bloom/Core/Base.h"

#ifdef BLOOM_PLATFORM_APPLE
#	define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <utl/utility.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"

using namespace bloom;

void* Window::nativeHandle() {
    return (__bridge void*)glfwGetCocoaWindow((GLFWwindow*)glfwWindowPtr.get());
}

void Window::setSwapchain(std::unique_ptr<Swapchain> _s) {
    _swapchain = std::move(_s);
    MetalSwapchain& mtlSwapchain =
        dynamic_cast<MetalSwapchain&>(swapchain());
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
}

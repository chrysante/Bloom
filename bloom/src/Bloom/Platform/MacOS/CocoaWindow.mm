#include "Bloom/Core/Base.hpp"

#ifdef BLOOM_PLATFORM_APPLE
#	define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <utl/utility.hpp>

#include "Bloom/Application/Window.hpp"
#include "Bloom/Platform/Metal/MetalSwapchain.h"


namespace bloom {

	void* Window::nativeHandle() {
		return (__bridge void*)glfwGetCocoaWindow((GLFWwindow*)glfwWindowPtr.get());
	}
	
	void Window::setSwapchain(std::unique_ptr<Swapchain> _s) {
		theSwapchain = std::move(_s);
		MetalSwapchain& mtlSwapchain = utl::down_cast<MetalSwapchain&>(*theSwapchain);
		
		NSWindow* nsWindow = (__bridge NSWindow*)nativeHandle();
		nsWindow.contentView.wantsLayer = true;
		[nsWindow.contentView setLayer:mtlSwapchain.layer];
	}
	
	void Window::platformInit() {
		NSWindow* nsWindow = (__bridge NSWindow*)nativeHandle();
		NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect: NSZeroRect
																	options: NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
																	  owner: nsWindow
																   userInfo: nil];
		
		[nsWindow.contentView addTrackingArea:trackingArea];
	}
	
	
	
}

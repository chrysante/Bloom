#import "AppViewController.h"

#import "KeyEventResponder.h"
#import "Event.h"

#include <utl/memory.hpp>
#include <utl/stdio.hpp>

#include "Bloom/Application/Application.hpp"
#include "Bloom/Platform/Metal/MetalRenderContext.hpp"

@implementation AppViewController

-(instancetype)initWithNibName:(nullable NSString *)nibNameOrNil bundle:(nullable NSBundle *)nibBundleOrNil
{
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	
	self.device = MTLCreateSystemDefaultDevice();
	self.commandQueue = [_device newCommandQueue];

	if (!self.device)
	{
		NSLog(@"Metal is not supported");
		abort();
	}
	
	self.application = createBloomApplication();
	
	return self;
}

-(void)dealloc
{
	using bloom::internal::AppInternals;
	AppInternals::shutdown(self.application);
	delete self.application;
}

-(MTKView *)mtkView
{
	return (MTKView*)self.view;
}

-(void)loadView
{
	self.view = [[MTKView alloc] initWithFrame:CGRectMake(0, 0, 1200, 720)];
}

-(void)viewDidLoad
{
	[super viewDidLoad];

	self.mtkView.device                = self.device;
	self.mtkView.delegate              = self;
	//self.mtkView.paused                = true;
	//self.mtkView.enableSetNeedsDisplay = true;
	
	// Add a tracking area in order to receive mouse events whenever the mouse is within the bounds of our view
	NSTrackingArea *trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect
																options:NSTrackingMouseMoved | NSTrackingInVisibleRect | NSTrackingActiveAlways
																  owner:self
															   userInfo:nil];
	[self.view addTrackingArea:trackingArea];
	
	self.keyEventResponder = [[BloomKeyEventResponder alloc] initWithFrame: NSZeroRect];

	
	
	
	self.inputContext = [[NSTextInputContext alloc] initWithClient: self.keyEventResponder];
	[self.view addSubview: self.keyEventResponder];
	
	// Some events do not raise callbacks of AppView in some circumstances (for example when CMD key is held down).
	// This monitor taps into global event stream and captures these events.
	NSEventMask eventMask = NSEventMaskFlagsChanged | NSEventMaskKeyDown | NSEventMaskKeyUp;
	__weak AppViewController* weakSelf = self;
	[NSEvent addLocalMonitorForEventsMatchingMask:eventMask handler:^NSEvent * _Nullable(NSEvent *event)
	{
		using bloom::internal::AppInternals;
		
		auto const type =
			event.type == NSEventTypeKeyDown ?
			bloom::EventType::keyDown :
			event.type == NSEventTypeKeyUp ?
			bloom::EventType::keyUp :
			bloom::EventType::keyFlagsChanged;
		
		AppInternals::handleEvent(weakSelf.application, bloom::Event(type, bloom::toKeyEvent(event)));
		
		return event;
	}];
	
	auto renderContext = utl::make_unique_ref<bloom::MetalRenderContext>((MTL::Device*)CFBridgingRetain(self.device),
																		 (__bridge void*)self);
	
	using bloom::internal::AppInternals;
	AppInternals::init(self.application, std::move(renderContext));
}

-(void)drawInMTKView:(MTKView*)view
{
	using bloom::internal::AppInternals;
	AppInternals::tick(self.application);
//	
//	
//	{
//		NSWindow* window = self.view.window;
//		
//		NSRect frame = [window frame];
//		frame.size = { 500, 500 };
//		[window setFrame: frame display: YES animate: NO];
//
//	}
	
}

-(void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size
{
	
}

-(void)handleEvent:(bloom::Event)bloomEvent withEvent: (NSEvent*)event {
	using bloom::internal::AppInternals;
	AppInternals::handleEvent(self.application, bloomEvent);
}

-(void)mouseDown:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::leftMouseDown,
		toMouseDownEvent(event)
	} withEvent:event];
}
-(void)rightMouseDown:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::rightMouseDown,
		toMouseDownEvent(event)
	} withEvent:event];
}
-(void)otherMouseDown:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::otherMouseDown,
		toMouseDownEvent(event)
	} withEvent:event];
}
-(void)mouseUp:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::leftMouseUp,
		toMouseUpEvent(event)
	} withEvent:event];
}
-(void)rightMouseUp:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::rightMouseUp,
		toMouseUpEvent(event)
	} withEvent:event];
}
-(void)otherMouseUp:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::otherMouseUp,
		toMouseUpEvent(event)
	} withEvent:event];
}
-(void)mouseMoved:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::mouseMoved,
		toMouseMoveEvent(event)
	} withEvent:event];
}
-(void)rightMouseMoved:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::mouseMoved,
		toMouseMoveEvent(event)
	} withEvent:event];
}
-(void)otherMouseMoved:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::mouseMoved,
		toMouseMoveEvent(event)
	} withEvent:event];
}
-(void)mouseDragged:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::leftMouseDragged,
		toMouseDraggedEvent(event)
	} withEvent:event];
}
-(void)rightMouseDragged:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::rightMouseDragged,
		toMouseDraggedEvent(event)
	} withEvent:event];
}
-(void)otherMouseDragged:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::otherMouseDragged,
		toMouseDraggedEvent(event)
	} withEvent:event];
}
-(void)scrollWheel:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::scrollWheel,
		toScrollEvent(event)
	} withEvent:event];
}
-(void)magnifyWithEvent:(NSEvent *)event {
	using namespace bloom;
	[self handleEvent:Event{
		bloom::EventType::magnify,
		toMagnificationEvent(event)
	} withEvent:event];
}


@end

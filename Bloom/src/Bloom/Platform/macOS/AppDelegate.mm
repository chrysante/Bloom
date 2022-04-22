#import "AppDelegate.h"
#import "AppViewController.h"


@implementation AppDelegate

-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
	return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	
}

-(instancetype)init
{
	if (self = [super init])
	{
		NSViewController *rootViewController = [[AppViewController alloc] initWithNibName:nil bundle:nil];
		self.window = [[NSWindow alloc] initWithContentRect:NSZeroRect
												  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
													backing:NSBackingStoreBuffered
													  defer:NO];
		self.window.contentViewController = rootViewController;
		[self.window orderFront:self];
		[self.window center];
	}
	return self;
}

@end

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include <Cocoa/Cocoa.h>

#include <utl/functional.hpp>

#import "KeyEventResponder.h"

namespace bloom {
	class Application;
}

@interface AppViewController : NSViewController
@end

@interface AppViewController () <MTKViewDelegate>
@property (nonatomic, readonly) MTKView *mtkView;
@property (nonatomic, strong) id <MTLDevice> device;
@property (nonatomic, strong) id <MTLCommandQueue> commandQueue;

@property BloomKeyEventResponder* keyEventResponder;
@property NSTextInputContext*  inputContext;

@property bloom::Application* application;

@end


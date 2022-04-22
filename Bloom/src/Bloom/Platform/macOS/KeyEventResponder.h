#import <Cocoa/Cocoa.h>
#include <utl/functional.hpp>

@interface BloomKeyEventResponder: NSView<NSTextInputClient>
@property (readwrite) utl::function<bool(NSEvent* _Nonnull)> keyEventHandler;
@property (readwrite) utl::function<void(id _Nonnull, NSRange)> insertTextHandler;
@end

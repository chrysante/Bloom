#import "KeyEventResponder.h"

#include <mtl/mtl.hpp>

using namespace mtl::short_types;

@implementation BloomKeyEventResponder
{
	float2 _position;
	NSRect _imeRect;
}

- (void)setImePosX:(float)posX imePosY:(float)posY
{
	_position = { posX, posY };
}

- (void)updateImePosWithView:(NSView *)view
{
	NSWindow *window = view.window;
	if (!window)
		return;
	NSRect contentRect = [window contentRectForFrameRect:window.frame];
	NSRect rect = NSMakeRect(_position.x, contentRect.size.height - _position.y, 0, 0);
	_imeRect = [window convertRectToScreen:rect];
}

- (void)viewDidMoveToWindow
{
	// Ensure self is a first responder to receive the input events.
	[self.window makeFirstResponder:self];
}

- (void)keyDown:(NSEvent*)event
{
	if (!self.keyEventHandler(event))
		[super keyDown:event];

	// Call to the macOS input manager system.
	[self interpretKeyEvents:@[event]];
}

- (void)keyUp:(NSEvent*)event
{
	if (!self.keyEventHandler(event))
		[super keyUp:event];
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
	if (self.insertTextHandler) {
		self.insertTextHandler(aString, replacementRange);
	}
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)doCommandBySelector:(SEL)myselector
{
}

- (nullable NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
	return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(nullable NSRangePointer)actualRange
{
	return _imeRect;
}

- (BOOL)hasMarkedText
{
	return NO;
}

- (NSRange)markedRange
{
	return NSMakeRange(NSNotFound, 0);
}

- (NSRange)selectedRange
{
	return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(nonnull id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
}

- (void)unmarkText
{
}

- (nonnull NSArray<NSAttributedStringKey>*)validAttributesForMarkedText
{
	return @[];
}

@end

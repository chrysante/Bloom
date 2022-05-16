#pragma once

#include "Bloom/Application/InputEvent.hpp"

#import <Cocoa/Cocoa.h>

namespace bloom {
	
	Key translateKeyCode(int keyCode);

	InputEventBase toInputEventBase(NSEvent*);

	KeyEvent toKeyEvent(NSEvent*);

	MouseEvent toMouseEvent(NSEvent*);
	
	MouseDownEvent toMouseDownEvent(NSEvent*);

	MouseUpEvent toMouseUpEvent(NSEvent*);

	MouseDragEvent toMouseDraggedEvent(NSEvent*);

	ScrollEvent toScrollEvent(NSEvent*);

	MagnificationEvent toMagnificationEvent(NSEvent*);

	MouseMoveEvent toMouseMoveEvent(NSEvent*);
	
}

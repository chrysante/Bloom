#pragma once

#include "Bloom/Application/Event.hpp"

#import <Cocoa/Cocoa.h>

namespace bloom {
	
	Key translateKeyCode(int keyCode);

	EventBase toEventBase(NSEvent*);

	KeyEvent toKeyEvent(NSEvent*);

	MouseEvent toMouseEvent(NSEvent*);
	
	MouseDownEvent toMouseDownEvent(NSEvent*);

	MouseUpEvent toMouseUpEvent(NSEvent*);

	MouseDragEvent toMouseDraggedEvent(NSEvent*);

	ScrollEvent toScrollEvent(NSEvent*);

	MagnificationEvent toMagnificationEvent(NSEvent*);

	MouseMoveEvent toMouseMoveEvent(NSEvent*);
	
}

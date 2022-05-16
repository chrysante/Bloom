#include "InputEvent.h"

#import <Carbon/Carbon.h>

#include <utl/stdio.hpp>

namespace bloom {
	
	Key translateKeyCode(int keyCode) {
		switch (keyCode) {
			case kVK_ANSI_A:              return Key::A;
			case kVK_ANSI_S:              return Key::S;
			case kVK_ANSI_D:              return Key::D;
			case kVK_ANSI_F:              return Key::F;
			case kVK_ANSI_H:              return Key::H;
			case kVK_ANSI_G:              return Key::G;
			case kVK_ANSI_Z:              return Key::Z;
			case kVK_ANSI_X:              return Key::X;
			case kVK_ANSI_C:              return Key::C;
			case kVK_ANSI_V:              return Key::V;
			case kVK_ANSI_B:              return Key::B;
			case kVK_ANSI_Q:              return Key::Q;
			case kVK_ANSI_W:              return Key::W;
			case kVK_ANSI_E:              return Key::E;
			case kVK_ANSI_R:              return Key::R;
			case kVK_ANSI_Y:              return Key::Y;
			case kVK_ANSI_T:              return Key::T;
			case kVK_ANSI_1:              return Key::_1;
			case kVK_ANSI_2:              return Key::_2;
			case kVK_ANSI_3:              return Key::_3;
			case kVK_ANSI_4:              return Key::_4;
			case kVK_ANSI_6:              return Key::_6;
			case kVK_ANSI_5:              return Key::_5;
			case kVK_ANSI_Equal:          return Key::equal;
			case kVK_ANSI_9:              return Key::_9;
			case kVK_ANSI_7:              return Key::_7;
			case kVK_ANSI_Minus:          return Key::minus;
			case kVK_ANSI_8:              return Key::_8;
			case kVK_ANSI_0:              return Key::_0;
			case kVK_ANSI_RightBracket:   return Key::rightBracket;
			case kVK_ANSI_O:              return Key::O;
			case kVK_ANSI_U:              return Key::U;
			case kVK_ANSI_LeftBracket:    return Key::leftBracket;
			case kVK_ANSI_I:              return Key::I;
			case kVK_ANSI_P:              return Key::P;
			case kVK_ANSI_L:              return Key::L;
			case kVK_ANSI_J:              return Key::J;
			case kVK_ANSI_Quote:          return Key::apostrophe;
			case kVK_ANSI_K:              return Key::K;
			case kVK_ANSI_Semicolon:      return Key::semicolon;
			case kVK_ANSI_Backslash:      return Key::backslash;
			case kVK_ANSI_Comma:          return Key::comma;
			case kVK_ANSI_Slash:          return Key::slash;
			case kVK_ANSI_N:              return Key::N;
			case kVK_ANSI_M:              return Key::M;
			case kVK_ANSI_Period:         return Key::period;
			case kVK_ANSI_Grave:          return Key::graveAccent;
			case kVK_ANSI_KeypadDecimal:  return Key::keypadDecimal;
			case kVK_ANSI_KeypadMultiply: return Key::keypadMultiply;
			case kVK_ANSI_KeypadPlus:     return Key::keypadAdd;
			case kVK_ANSI_KeypadClear:    return Key::numLock;
			case kVK_ANSI_KeypadDivide:   return Key::keypadDivide;
			case kVK_ANSI_KeypadEnter:    return Key::keypadEnter;
			case kVK_ANSI_KeypadMinus:    return Key::keypadSubtract;
			case kVK_ANSI_KeypadEquals:   return Key::keypadEqual;
			case kVK_ANSI_Keypad0:        return Key::keypad0;
			case kVK_ANSI_Keypad1:        return Key::keypad1;
			case kVK_ANSI_Keypad2:        return Key::keypad2;
			case kVK_ANSI_Keypad3:        return Key::keypad3;
			case kVK_ANSI_Keypad4:        return Key::keypad4;
			case kVK_ANSI_Keypad5:        return Key::keypad5;
			case kVK_ANSI_Keypad6:        return Key::keypad6;
			case kVK_ANSI_Keypad7:        return Key::keypad7;
			case kVK_ANSI_Keypad8:        return Key::keypad8;
			case kVK_ANSI_Keypad9:        return Key::keypad9;
			case kVK_Return:              return Key::enter;
			case kVK_Tab:                 return Key::tab;
			case kVK_Space:               return Key::space;
			case kVK_Delete:              return Key::backspace;
			case kVK_Escape:              return Key::escape;
			case kVK_CapsLock:            return Key::capsLock;
			case kVK_Control:             return Key::leftCtrl;
			case kVK_Shift:               return Key::leftShift;
			case kVK_Option:              return Key::leftAlt;
			case kVK_Command:             return Key::leftSuper;
			case kVK_RightControl:        return Key::rightCtrl;
			case kVK_RightShift:          return Key::rightShift;
			case kVK_RightOption:         return Key::rightAlt;
			case kVK_RightCommand:        return Key::rightSuper;
			case kVK_F5:                  return Key::F5;
			case kVK_F6:                  return Key::F6;
			case kVK_F7:                  return Key::F7;
			case kVK_F3:                  return Key::F3;
			case kVK_F8:                  return Key::F8;
			case kVK_F9:                  return Key::F9;
			case kVK_F11:                 return Key::F11;
			case kVK_F13:                 return Key::printScreen;
			case kVK_F10:                 return Key::F10;
			case 0x6E:                    return Key::menu;
			case kVK_F12:                 return Key::F12;
			case kVK_Help:                return Key::insert;
			case kVK_Home:                return Key::home;
			case kVK_PageUp:              return Key::pageUp;
			case kVK_ForwardDelete:       return Key::delete_;
			case kVK_F4:                  return Key::F4;
			case kVK_End:                 return Key::end;
			case kVK_F2:                  return Key::F2;
			case kVK_PageDown:            return Key::pageDown;
			case kVK_F1:                  return Key::F1;
			case kVK_LeftArrow:           return Key::leftArrow;
			case kVK_RightArrow:          return Key::rightArrow;
			case kVK_DownArrow:           return Key::downArrow;
			case kVK_UpArrow:             return Key::upArrow;
			default:                      return Key::none;
	   }
	}

	static bool isTrackpadScroll(NSEvent* event) {
		return event.phase != 0 || event.momentumPhase != 0;
	}

	InputEventBase toInputEventBase(NSEvent* event) {
		return {
			.modifierFlags    = (InputModifierFlags)((unsigned)event.modifierFlags >> 16)
		};
	}

	KeyEvent toKeyEvent(NSEvent* event) {
		KeyEvent result{ toInputEventBase(event) };
		result.key = translateKeyCode(event.keyCode);
		if (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp) {
			result.repeat = event.isARepeat;
		}
		return result;
	}

	MouseEvent toMouseEvent(NSEvent* event) {
		double const height = event.window.contentLayoutRect.size.height;
		mtl::double2 const locationInWindow = { event.locationInWindow.x, height - event.locationInWindow.y };
		
		return {
			toInputEventBase(event),
			.locationInWindow = locationInWindow
		};
	}
	
	MouseDownEvent toMouseDownEvent(NSEvent* event) {
		return {
			toMouseEvent(event),
			.clickCount = event.clickCount
		};
	}

	MouseUpEvent toMouseUpEvent(NSEvent* event) {
		return {
			toMouseEvent(event)
		};
	}

	MouseDragEvent toMouseDraggedEvent(NSEvent* event) {
		return {
			toMouseEvent(event),
			.offset = { event.deltaX, event.deltaY }
		};
	}

	ScrollEvent toScrollEvent(NSEvent* event) {
		return {
			toMouseEvent(event),
			.offset     = { event.scrollingDeltaX, event.scrollingDeltaY },
			.isTrackpad = isTrackpadScroll(event)
		};
	}

	MagnificationEvent toMagnificationEvent(NSEvent* event) {
		return {
			toMouseEvent(event),
			.offset = event.magnification
		};
	}

	MouseMoveEvent toMouseMoveEvent(NSEvent* event) {
		return {
			toMouseEvent(event),
			.offset = { event.deltaX, event.deltaY }
		};
	}

	
}

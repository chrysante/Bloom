#include "Input.hpp"

#include "InputEvent.hpp"

namespace bloom {
	
	void Input::_setFromEvent(InputEvent const& event) {
		auto const modFlags = event.modifierFlags();
		
		
		_setKey(Key::capsLock, test(modFlags & InputModifierFlags::capsLock));
		_setKey(Key::leftShift, test(modFlags & InputModifierFlags::shift));
		_setKey(Key::leftCtrl, test(modFlags & InputModifierFlags::control));
		_setKey(Key::leftAlt, test(modFlags & InputModifierFlags::option));
		_setKey(Key::leftSuper, test(modFlags & InputModifierFlags::super));
//		_setKey(Key::numLock, test(modFlags & InputModifierFlags::numericPad));
//		_setKey(Key::, test(modFlags & InputModifierFlags::help));
//		_setKey(Key::, test(modFlags & InputModifierFlags::function));
		
		switch (event.type()) {
			case InputEventType::leftMouseDown:
				_setMouseButton(MouseButton::left, 1);
				break;
			case InputEventType::rightMouseDown:
				_setMouseButton(MouseButton::right, 1);
				break;
			case InputEventType::otherMouseDown:
				_setMouseButton(MouseButton::other, 1);
				break;
			case InputEventType::leftMouseUp:
				_setMouseButton(MouseButton::left, 0);
				break;
			case InputEventType::rightMouseUp:
				_setMouseButton(MouseButton::right, 0);
				break;
			case InputEventType::otherMouseUp:
				_setMouseButton(MouseButton::other, 0);
				break;
			
			case InputEventType::mouseMoved:
				_mouseOffset = std::get<MouseMoveEvent>(event._union).offset;
				break;
				
			case InputEventType::leftMouseDragged:  [[ fallthrough ]];
			case InputEventType::rightMouseDragged: [[ fallthrough ]];
			case InputEventType::otherMouseDragged:
				_mouseOffset = std::get<MouseDragEvent>(event._union).offset;
				break;
				
			case InputEventType::scrollWheel:
				_scrollOffset = std::get<ScrollEvent>(event._union).offset;
				break;
				
			case InputEventType::magnify:
				break;
			
			case InputEventType::keyDown:
				_setKey(event.get<KeyEvent>().key, true);
				break;
			case InputEventType::keyUp:
				_setKey(event.get<KeyEvent>().key, false);
				break;
				
			default:
				break;
		}
	}
	
	
}

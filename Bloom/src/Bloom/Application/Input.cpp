#include "Input.hpp"

#include "Event.hpp"

namespace bloom {
	
	void Input::_setFromEvent(Event const& event) {
		try {
		switch (event.type()) {
			case EventType::leftMouseDown:
				_setMouseButton(MouseButton::left, 1);
				break;
			case EventType::rightMouseDown:
				_setMouseButton(MouseButton::right, 1);
				break;
			case EventType::otherMouseDown:
				_setMouseButton(MouseButton::other, 1);
				break;
			case EventType::leftMouseUp:
				_setMouseButton(MouseButton::left, 0);
				break;
			case EventType::rightMouseUp:
				_setMouseButton(MouseButton::right, 0);
				break;
			case EventType::otherMouseUp:
				_setMouseButton(MouseButton::other, 0);
				break;
			
			case EventType::mouseMoved:
				_mouseOffset = std::get<MouseMoveEvent>(event._union).offset;
				break;
				
			case EventType::leftMouseDragged:  [[ fallthrough ]];
			case EventType::rightMouseDragged: [[ fallthrough ]];
			case EventType::otherMouseDragged:
				_mouseOffset = std::get<MouseDragEvent>(event._union).offset;
				break;
				
				
			case EventType::scrollWheel:
				_scrollOffset = std::get<ScrollEvent>(event._union).offset;
				break;
				
			case EventType::magnify:
				break;
			
			case EventType::keyDown:
				_setKey(event.get<KeyEvent>().key, true);
				break;
			case EventType::keyUp:
				_setKey(event.get<KeyEvent>().key, false);
				break;
				
			default:
				break;
		}
		}
		catch (std::bad_variant_access const& e) {
			bloomLog("{}", e.what());
			bloomDebugbreak();
			std::terminate();
		}
	}
	
	
}

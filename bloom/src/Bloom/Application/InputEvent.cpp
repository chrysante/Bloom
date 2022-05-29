#include "InputEvent.hpp"

#include <ostream>

#include <GLFW/glfw3.h>

namespace bloom {
	
	std::string_view toString(InputEventType e) {
		return std::array{
			"mouseDown",
			"rightMouseDown",
			"otherMouseDown",
			"mouseUp",
			"rightMouseUp",
			"otherMouseUp",
			"mouseMoved",
			"mouseDragged",
			"rightMouseMoved",
			"rightMouseDragged",
			"otherMouseMoved",
			"otherMouseDragged",
			"scrollWheel",
			"magnify"
		}[(unsigned)e];
	}
	
	std::ostream& operator<<(std::ostream& str, InputEventType e) {
		return str << toString(e);
	}
	
	InputEvent inputEventFromGLFWMouseButton(Input const& input, int buttonCode, int action, int mods) {
		auto const [type, button] = [&]{
			switch (buttonCode) {
				case GLFW_MOUSE_BUTTON_LEFT:
					return std::pair{
						action == GLFW_PRESS ? InputEventType::leftMouseDown : InputEventType::leftMouseUp,
						MouseButton::left
					};
				case GLFW_MOUSE_BUTTON_RIGHT:
					return std::pair{
						action == GLFW_PRESS ? InputEventType::rightMouseDown : InputEventType::rightMouseUp,
						MouseButton::right
					};
				case GLFW_MOUSE_BUTTON_MIDDLE:
					return std::pair{
						action == GLFW_PRESS ? InputEventType::otherMouseDown : InputEventType::otherMouseUp,
						MouseButton::other
					};
					
				default:
					return std::pair{
						InputEventType::none,
						MouseButton::none
					};
			}
		}();
		
		MouseEvent event;
		event.modifierFlags = input.modFlags();
		event.locationInWindow = input.mousePosition();
		
		switch (action) {
			case GLFW_PRESS: {
				MouseDownEvent downEvent{ event };
				downEvent.button = button;
				downEvent.clickCount = 1; // TODO: get correct click count
				return InputEvent(type, downEvent);
			}
				
			case GLFW_RELEASE: {
				MouseUpEvent upEvent{ event };
				upEvent.button = button;
				return InputEvent(type, upEvent);
			}
			default: bloomDebugfail("Are these even generated?");
		}
	}
	
	InputEvent inputEventFromGLFWCursorPos(Input const& input, double xpos, double ypos) {
		MouseMoveEvent event;
		event.modifierFlags = input.modFlags();
		event.locationInWindow = input.mousePosition();
		event.offset = input.mouseOffset();
		

		MouseDragEvent dragEvent{ event };
		
		if (input.mouseDown(MouseButton::left)) {
			dragEvent.button = MouseButton::left;
			return InputEvent(InputEventType::leftMouseDragged, dragEvent);
		}
		else if (input.mouseDown(MouseButton::right)) {
			dragEvent.button = MouseButton::right;
			return InputEvent(InputEventType::rightMouseDragged, dragEvent);
		}
		else if (input.mouseDown(MouseButton::other)) {
			dragEvent.button = MouseButton::other;
			return InputEvent(InputEventType::otherMouseDragged, dragEvent);
		}
		else {
			return InputEvent(InputEventType::mouseMoved, event);
		}
	}
	
	InputEvent inputEventFromGLFWScroll(Input const& input, double xoffset, double yoffset) {
		ScrollEvent event;
		event.modifierFlags = input.modFlags();
		event.locationInWindow = input.mousePosition();
		event.offset = { xoffset, yoffset };
		return InputEvent(InputEventType::scrollWheel, event);
	}
	
	InputEvent inputEventFromGLFWKey(Input const& input, int keyCode, int /* scancode */, int action, int mods) {
		Key key = keyFromGLFW(keyCode);
		KeyEvent event;
		event.modifierFlags = input.modFlags();
		event.repeat = input.keyDownRepeatCount(key); // TODO: Fix this
		event.key = key;
		
		switch (action) {
			case GLFW_PRESS:
			case GLFW_REPEAT:
				return InputEvent(InputEventType::keyDown, event);
				
			case GLFW_RELEASE: {
				return InputEvent(InputEventType::keyUp, event);
			}
			default: bloomDebugfail("Are these even generated?");
		}
	}
	
}

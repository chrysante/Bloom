#include "InputEvent.hpp"

#include <ostream>

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
	
}

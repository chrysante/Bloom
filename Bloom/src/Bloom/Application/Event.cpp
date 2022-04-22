#include "Event.hpp"

#include <ostream>

namespace bloom {
	
	std::string_view toString(EventType e) {
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
	
	std::ostream& operator<<(std::ostream& str, EventType e) {
		return str << toString(e);
	}
	
}

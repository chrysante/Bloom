#include "Bloom/Core/Autorelease.hpp"

namespace bloom {

	void autoreleased(utl::function<void()> const& block) {
		@autoreleasepool {
			block();
		}
	}
	
}


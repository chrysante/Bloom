#pragma once

#include <utl/functional.hpp>

namespace bloom {
	
	struct BLOOM_API CustomCommand {
		utl::function<void()> function;
	};
	
	
	
}

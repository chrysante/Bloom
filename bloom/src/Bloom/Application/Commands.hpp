#pragma once

#include <utl/functional.hpp>
#include <concepts>

namespace bloom {
	
	struct BLOOM_API CustomCommand {
		CustomCommand(std::invocable auto&& block): function(UTL_FORWARD(block)) {}
		utl::function<void()> function;
	};
	
	
	
}

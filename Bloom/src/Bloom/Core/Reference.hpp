#pragma once

#include <memory>
#include <concepts>
#include <utl/common.hpp>

namespace bloom {
	
	template <typename T>
	using Reference = std::shared_ptr<T>;
	
	template <typename T, typename... Args> requires std::constructible_from<T, Args...>
	Reference<T> allocateRef(Args&&... args) {
		return std::make_shared<T>(UTL_FORWARD(args)...);
	}
	
}

#pragma once

#include <memory>

namespace bloom {
	
	template <typename T>
	using Reference = std::shared_ptr<T>;
	
}

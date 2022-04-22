#pragma once

#include "Bloom/Core/Base.hpp"
#include <utl/functional.hpp>
#include <concepts>

namespace bloom {
	
	class BLOOM_API RenderPassHandle {
	public:
		explicit RenderPassHandle(std::invocable auto&& wait): _wait(std::move(wait)) {}
		void wait() const { _wait(); }
		
	private:
		utl::function<void()> _wait;
	};
	
}

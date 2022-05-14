#pragma once

#include "Bloom/Core/Base.hpp"
#include <utl/functional.hpp>
#include <concepts>

namespace bloom {
	
	class BLOOM_API RenderPassHandle {
	public:
		RenderPassHandle() = default;
		explicit RenderPassHandle(std::invocable auto&& wait): _wait(std::move(wait)) {}
		void wait() const { if (_wait) _wait(); }
		
	private:
		utl::function<void()> _wait;
	};
	
}

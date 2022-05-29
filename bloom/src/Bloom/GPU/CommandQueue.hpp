#pragma once

#include "RenderContext.hpp"
#include "ComputeContext.hpp"
#include "BlitContext.hpp"

#include <memory>

namespace bloom {
	
	class BLOOM_API CommandQueue {
	public:
		virtual ~CommandQueue() = default;
		virtual std::unique_ptr<RenderContext> createRenderContext() = 0;
		virtual std::unique_ptr<ComputeContext> createComputeContext() = 0;
		virtual std::unique_ptr<BlitContext> createBlitContext() = 0;
	};
	
}

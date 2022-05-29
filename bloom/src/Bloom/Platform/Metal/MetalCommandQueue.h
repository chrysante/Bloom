#pragma once

#include "Bloom/GPU/CommandQueue.hpp"

#import <Metal/Metal.h>

namespace bloom {
	
	class BLOOM_API MetalCommandQueue: public CommandQueue {
	public:
		MetalCommandQueue(id<MTLCommandQueue>);
		
		std::unique_ptr<RenderContext> createRenderContext() override;
		std::unique_ptr<ComputeContext> createComputeContext() override;
		std::unique_ptr<BlitContext> createBlitContext() override;
		
		id<MTLCommandQueue> queue;
	};
	
}

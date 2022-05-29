#include "MetalCommandQueue.h"

#include "MetalRenderContext.h"
#include "MetalComputeContext.h"
#include "MetalBlitContext.h"

namespace bloom {
	
	MetalCommandQueue::MetalCommandQueue(id<MTLCommandQueue> q) {
		queue = q;
	}
	
	std::unique_ptr<RenderContext> MetalCommandQueue::createRenderContext() {
		return std::make_unique<MetalRenderContext>([queue commandBuffer]);
	}
	
	std::unique_ptr<ComputeContext> MetalCommandQueue::createComputeContext() {
		return std::make_unique<MetalComputeContext>([queue commandBuffer]);
	}
	
	std::unique_ptr<BlitContext> MetalCommandQueue::createBlitContext() {
		return std::make_unique<MetalBlitContext>([queue commandBuffer]);
	}
	
}

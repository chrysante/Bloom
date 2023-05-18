#include "Bloom/Platform/Metal/MetalCommandQueue.h"

#include "Bloom/Platform/Metal/MetalRenderContext.h"
#include "Bloom/Platform/Metal/MetalComputeContext.h"
#include "Bloom/Platform/Metal/MetalBlitContext.h"

using namespace bloom;

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

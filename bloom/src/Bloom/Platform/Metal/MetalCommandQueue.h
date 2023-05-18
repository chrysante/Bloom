#pragma once

#import <Metal/Metal.h>

#include "Bloom/GPU/CommandQueue.hpp"

namespace bloom {

class BLOOM_API MetalCommandQueue: public CommandQueue {
public:
    MetalCommandQueue(id<MTLCommandQueue>);

    std::unique_ptr<RenderContext> createRenderContext() override;
    std::unique_ptr<ComputeContext> createComputeContext() override;
    std::unique_ptr<BlitContext> createBlitContext() override;

    id<MTLCommandQueue> queue;
};

} // namespace bloom

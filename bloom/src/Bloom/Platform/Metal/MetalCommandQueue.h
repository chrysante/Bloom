#ifndef BLOOM_PLATFORM_METAL_METALCOMMANDQUEUE_H
#define BLOOM_PLATFORM_METAL_METALCOMMANDQUEUE_H

#import <Metal/Metal.h>

#include "Bloom/GPU/CommandQueue.h"

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

#endif // BLOOM_PLATFORM_METAL_METALCOMMANDQUEUE_H

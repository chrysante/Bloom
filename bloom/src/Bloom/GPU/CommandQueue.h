#ifndef BLOOM_GPU_COMMANDQUEUE_H
#define BLOOM_GPU_COMMANDQUEUE_H

#include <memory>

#include "Bloom/GPU/BlitContext.h"
#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/RenderContext.h"

namespace bloom {

///
class BLOOM_API CommandQueue {
public:
    virtual ~CommandQueue() = default;

    /// Creates a render context that commits to this command queue
    virtual std::unique_ptr<RenderContext> createRenderContext() = 0;

    /// Creates a compute context that commits to this command queue
    virtual std::unique_ptr<ComputeContext> createComputeContext() = 0;

    /// Creates a blit context that commits to this command queue
    virtual std::unique_ptr<BlitContext> createBlitContext() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_COMMANDQUEUE_H

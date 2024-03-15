#ifndef BLOOM_GPU_COMMANDQUEUE_H
#define BLOOM_GPU_COMMANDQUEUE_H

#include <memory>

#include "Bloom/GPU/BlitContext.h"
#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/RenderContext.h"

namespace bloom {

class BLOOM_API CommandQueue {
public:
    virtual ~CommandQueue() = default;
    virtual std::unique_ptr<RenderContext> createRenderContext() = 0;
    virtual std::unique_ptr<ComputeContext> createComputeContext() = 0;
    virtual std::unique_ptr<BlitContext> createBlitContext() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_COMMANDQUEUE_H

#pragma once

#include <memory>

#include "Bloom/GPU/BlitContext.hpp"
#include "Bloom/GPU/ComputeContext.hpp"
#include "Bloom/GPU/RenderContext.hpp"

namespace bloom {

class BLOOM_API CommandQueue {
public:
    virtual ~CommandQueue() = default;
    virtual std::unique_ptr<RenderContext> createRenderContext() = 0;
    virtual std::unique_ptr<ComputeContext> createComputeContext() = 0;
    virtual std::unique_ptr<BlitContext> createBlitContext() = 0;
};

} // namespace bloom

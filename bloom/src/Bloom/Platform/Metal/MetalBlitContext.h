#pragma once

#include <Metal/Metal.h>

#include "Bloom/GPU/BlitContext.hpp"

namespace bloom {

class MetalBlitContext: public BlitContext {
public:
    MetalBlitContext(id<MTLCommandBuffer>);
    void commit() override;

    id<MTLCommandBuffer> commandBuffer;
};

} // namespace bloom

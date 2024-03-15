#ifndef BLOOM_PLATFORM_METAL_METALBLITCONTEXT_H
#define BLOOM_PLATFORM_METAL_METALBLITCONTEXT_H

#include <Metal/Metal.h>

#include "Bloom/GPU/BlitContext.h"

namespace bloom {

class MetalBlitContext: public BlitContext {
public:
    MetalBlitContext(id<MTLCommandBuffer>);
    void commit() override;

    id<MTLCommandBuffer> commandBuffer;
};

} // namespace bloom

#endif // BLOOM_PLATFORM_METAL_METALBLITCONTEXT_H

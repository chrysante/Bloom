#ifndef BLOOM_PLATFORM_METAL_METALBLITCONTEXT_H
#define BLOOM_PLATFORM_METAL_METALBLITCONTEXT_H

#include <Metal/Metal.h>

#include "Bloom/GPU/BlitContext.h"

namespace bloom {

class MetalBlitContext: public BlitContext {
public:
    MetalBlitContext(id<MTLCommandBuffer>);
    void begin() override;
    void end() override;
    void commit() override;
    void waitUntilCompleted() override;
    void synchronizeTexture(TextureView texture, size_t slice,
                            size_t level) override;

    id<MTLCommandBuffer> commandBuffer;
    id<MTLBlitCommandEncoder> commandEncoder;
};

} // namespace bloom

#endif // BLOOM_PLATFORM_METAL_METALBLITCONTEXT_H

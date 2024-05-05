#ifndef BLOOM_GPU_BLITCONTEXT_H
#define BLOOM_GPU_BLITCONTEXT_H

#include "Bloom/GPU/Fwd.h"

namespace bloom {

class BlitContext {
public:
    virtual ~BlitContext() = default;

    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void commit() = 0;
    virtual void waitUntilCompleted() = 0;

    virtual void synchronizeTexture(TextureView texture, size_t slice,
                                    size_t level) = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_BLITCONTEXT_H

#ifndef BLOOM_GRAPHICS_RENDERER_BLOOMFRAMEBUFFER_H
#define BLOOM_GRAPHICS_RENDERER_BLOOMFRAMEBUFFER_H

#include "Bloom/Core/Core.h"

namespace bloom {

struct BLOOM_API BloomFramebuffer {
    static constexpr int numDSMipLevels = 8;
    static constexpr int numUSMipLevels = numDSMipLevels - 1;

    TextureHandle downsample;
    std::array<TextureHandle, numDSMipLevels> downsampleMips;
    TextureHandle upsample;
    std::array<TextureHandle, numUSMipLevels> upsampleMips;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_BLOOMFRAMEBUFFER_H

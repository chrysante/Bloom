#ifndef BLOOM_GRAPHICS_TEXTURE_H
#define BLOOM_GRAPHICS_TEXTURE_H

#include <vector>

#include "Bloom/Asset/Asset.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

enum class TexturePixelFormat { RGBA8 };

class Texture2D: public Asset {
public:
    BL_DEFINE_ASSET_CTOR(Texture2D, Asset)

    TexturePixelFormat pixelFormat;
    vml::usize2 size;
    std::vector<uint8_t> data;
    TextureHandle GPUHandle;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_TEXTURE_H

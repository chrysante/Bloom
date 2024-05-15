#ifndef BLOOM_GRAPHICS_TEXTURE_H
#define BLOOM_GRAPHICS_TEXTURE_H

#include <memory>
#include <vector>

#include <vml/vml.hpp>

#include "Bloom/Asset/Asset.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

class Texture2D: public Asset {
public:
    BL_DEFINE_ASSET_CTOR(Texture2D, Asset)

    size_t getPixelDataByteSize() const;

    TextureType type = TextureType::Texture2D;
    PixelFormat pixelFormat = PixelFormat::RGBA8Unorm;
    vml::usize2 size = 0;

    /// If this is set, this has `size.x * size.y * <size-of-single-pixel> *
    /// <num-slices>` number of elements.
    /// `<num-slices>` is 1 for 2D textures and 6 for cubemaps
    std::unique_ptr<uint8_t[]> pixelData;
    TextureHandle GPUHandle;
    std::vector<vml::float3> irradianceSHCoefficients;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_TEXTURE_H

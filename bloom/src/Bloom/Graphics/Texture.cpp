#include "Bloom/Graphics/Texture.h"

#include <range/v3/numeric.hpp>
#include <range/v3/view.hpp>

using namespace bloom;
using namespace ranges::views;

static size_t numSlices(TextureType type) {
    switch (type) {
    case TextureType::Texture2D:
        return 1;
    case TextureType::CubeTexture:
        return 6;
    default:
        BL_UNIMPLEMENTED();
    }
}

static size_t exp2(size_t n) {
    size_t result = 1;
    for (size_t i = 0; i < n; ++i) {
        result *= 2;
    }
    return result;
}

static size_t sqr(size_t n) { return n * n; }

size_t Texture2D::getPixelDataByteSize() const {
    size_t numPixelsPerSlice = size.x * size.y;
    size_t numMips = storesMipMaps ? numMipMaps : 1;
    size_t numTotalPixels = ranges::accumulate(iota(size_t{ 0 }, numMips),
                                               size_t{ 0 }, ranges::plus{},
                                               [&](size_t mipLevel) {
        return numPixelsPerSlice / sqr(exp2(mipLevel));
    }) * numSlices(type);
    return numTotalPixels * pixelByteSize(pixelFormat);
}

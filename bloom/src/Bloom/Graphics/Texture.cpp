#include "Bloom/Graphics/Texture.h"

using namespace bloom;

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

size_t Texture2D::getPixelDataByteSize() const {
    return size.x * size.y * pixelByteSize(pixelFormat) * numSlices(type);
}

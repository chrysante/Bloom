#include "Bloom/GPU/HardwarePrimitives.h"

#include "Bloom/Platform/Metal/MetalObjects.h"

using namespace bloom;

void bloom::copyTextureToMemory(RenderAPI api, TextureView source, void* dest,
                                size_t bytesPerRow,
                                vml::AABB<uint32_t, 3> region,
                                size_t mipLevel) {
    switch (api) {
    case RenderAPI::Metal:
        metalCopyTextureToMemory(source, dest, bytesPerRow, region, mipLevel);
        break;
    default:
        break;
    }
}

void bloom::copyMemoryToTexture(RenderAPI api, TextureView dest, void* data,
                                size_t bytesPerRow,
                                vml::AABB<uint32_t, 3> region,
                                size_t mipLevel) {
    switch (api) {
    case RenderAPI::Metal:
        metalCopyMemoryToTexture(dest, data, bytesPerRow, region, mipLevel);
        break;
    default:
        break;
    }
}

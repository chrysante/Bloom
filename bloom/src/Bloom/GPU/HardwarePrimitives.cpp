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
    }
}

void bloom::copyTextureToMemory(RenderAPI api, TextureView source, void* dest,
                                size_t bytesPerRow, size_t bytesPerSlice,
                                vml::AABB<uint32_t, 3> region, size_t mipLevel,
                                size_t slice) {
    switch (api) {
    case RenderAPI::Metal:
        metalCopyTextureToMemory(source, dest, bytesPerRow, bytesPerSlice,
                                 region, mipLevel, slice);
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
    }
}

void bloom::copyMemoryToTexture(RenderAPI api, TextureView dest, void* data,
                                size_t bytesPerRow, size_t bytesPerImage,
                                vml::AABB<uint32_t, 3> region, size_t mipLevel,
                                size_t slice) {
    switch (api) {
    case RenderAPI::Metal:
        metalCopyMemoryToTexture(dest, data, bytesPerRow, bytesPerImage, region,
                                 mipLevel, slice);
        break;
    }
}

TextureHandle bloom::createSharedTextureView(
    RenderAPI api, TextureView texture, TextureType newType,
    PixelFormat newFormat, size_t firstMipLevel, size_t numMipLevels,
    size_t firstSlice, size_t numSlices) {
    switch (api) {
    case RenderAPI::Metal:
        return metalCreateSharedTextureView(texture, newType, newFormat,
                                            firstMipLevel, numMipLevels,
                                            firstSlice, numSlices);
        break;
    }
}

size_t bloom::pixelByteSize(PixelFormat pixelFormat) {
    switch (pixelFormat) {
    case PixelFormat::RGBA8Unorm:
        return 4;
    case PixelFormat::RGBA32Float:
        return 16;
    default:
        BL_UNIMPLEMENTED();
    }
}

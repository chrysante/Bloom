#include "Bloom/GPU/HardwarePrimitives.h"

#include "Bloom/Platform/Metal/MetalPrimitives.h"

using namespace bloom;

void bloom::copyData(RenderAPI api, TextureView source, void* dest,
                     size_t bytesPerRow, vml::AABB<uint32_t, 3> region,
                     size_t mipLevel) {
    switch (api) {
    case RenderAPI::Metal:
        metalCopyData(source, dest, bytesPerRow, region, mipLevel);
        break;
    default:
        break;
    }
}

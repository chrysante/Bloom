#ifndef BLOOM_PLATFORM_METAL_METALPRIMITIVES_H
#define BLOOM_PLATFORM_METAL_METALPRIMITIVES_H

#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

void metalCopyData(TextureView source, void* dest, size_t bytesPerRow,
                   vml::AABB<uint32_t, 3> region, size_t mipLevel);

} // namespace bloom

#endif // BLOOM_PLATFORM_METAL_METALPRIMITIVES_H

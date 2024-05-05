#include "Bloom/Platform/Metal/MetalPrimitives.h"

#include <Metal/Metal.h>

static MTLRegion toMTL(vml::AABB<uint32_t, 3> region) {
    auto pos = region.lower_bound();
    auto size = region.size();
    return { .origin = MTLOrigin{ pos.x, pos.y, pos.z },
             .size = MTLSize{ size.x, size.y, size.z } };
}

void bloom::metalCopyData(TextureView source, void* dest, size_t bytesPerRow,
                          vml::AABB<uint32_t, 3> region, size_t mipLevel) {
    id<MTLTexture> native = (__bridge id<MTLTexture>)source.nativeHandle();
    auto mtlRegion = toMTL(region);
    [native getBytes:dest
         bytesPerRow:bytesPerRow
          fromRegion:mtlRegion
         mipmapLevel:mipLevel];
}

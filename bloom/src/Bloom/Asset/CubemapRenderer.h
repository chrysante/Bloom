#ifndef BLOOM_ASSET_CUBEMAPRENDERER_H
#define BLOOM_ASSET_CUBEMAPRENDERER_H

#include "Bloom/GPU/Fwd.h"

namespace bloom {

/// Renders the equirectangular map into the cubemap
void renderCubemap(HardwareDevice& device, CommandBuffer& commandBuffer,
                   TextureHandle cubemap, TextureHandle equirectangular);

} // namespace bloom

#endif // BLOOM_ASSET_CUBEMAPRENDERER_H

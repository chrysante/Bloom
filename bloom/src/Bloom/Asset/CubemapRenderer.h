#ifndef BLOOM_ASSET_CUBEMAPRENDERER_H
#define BLOOM_ASSET_CUBEMAPRENDERER_H

#include <array>

#include <vml/vml.hpp>

#include "Bloom/GPU/Fwd.h"

namespace bloom {

/// Renders the equirectangular map into the cubemap
void renderCubemap(HardwareDevice& device, CommandBuffer& commandBuffer,
                   TextureHandle cubemap, TextureHandle equirectangular);

void renderIrradianceMap(HardwareDevice& device, CommandBuffer& commandBuffer,
                         TextureHandle dest, TextureHandle source);

std::array<vml::float3, 9> genLightingCoefficients(vml::float4 const* imageData,
                                                   vml::usize2 resolution,
                                                   bool applyWindowing);

} // namespace bloom

#endif // BLOOM_ASSET_CUBEMAPRENDERER_H

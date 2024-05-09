#ifndef POPPY_SHADERS_PBRSHADER_H
#define POPPY_SHADERS_PBRSHADER_H

#include <metal_stdlib>

#include "Bloom/Shader/SharedTypes.h"

namespace bloom {

struct PBRData {
    metal::float3 albedo;
    metal::float3 emissive;
    float metallic;
    float roughness;
    float ao;
};

metal::float3 PBRForwardShader(
    PBRData pbr, metal::float3 worldPosition, metal::float3 worldNormal,
    metal::float3 cameraPosition,
    device const metal::float4x4* lightSpaceTransforms,
    device const LightBufferContents& lights,
    device const ShadowRenderMetadata& shadowMetadata,
    metal::texture2d_array<float> shadowMaps);

} // namespace bloom

#endif // POPPY_SHADERS_PBRSHADER_H

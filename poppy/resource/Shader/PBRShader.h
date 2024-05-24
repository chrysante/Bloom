#ifndef SHADER_PBRSHADER_H
#define SHADER_PBRSHADER_H

#include <metal_stdlib>

#include "Shader/SharedTypes.h"

namespace bloom {

struct PBRData {
    metal::float3 albedo;
    metal::float3 emissive;
    float roughness;
    float metallic;
    float ao;
    metal::float3 normal;
};

metal::float3 PBRForwardShader(
    PBRData pbr, metal::float3 worldPosition, metal::float3 cameraPosition,
    device const metal::float4x4* lightSpaceTransforms,
    device const LightBufferContents& lights,
    metal::texture2d_array<float> shadowMaps,
    metal::texturecube<float> specularMap, metal::texture2d<float> brdfMap);

} // namespace bloom

#endif // SHADER_PBRSHADER_H

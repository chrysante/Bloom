#pragma once

#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "PBRShader.h"
#include <metal_stdlib>

namespace bloom {

metal::float3 calculatePointLight(PBRData pbrData, PointLight light,
                                  metal::float3 V, metal::float3 N,
                                  metal::float3 worldPosition);

metal::float3 calculateSpotlight(PBRData pbrData, SpotLight light,
                                 metal::float3 V, metal::float3 N,
                                 metal::float3 worldPosition);

metal::float3 calculateDirectionalLight(PBRData pbrData, DirectionalLight light,
                                        metal::float3 V, metal::float3 N);

float calculateDirLightShadow(metal::texture2d_array<float> shadowMap,
                              metal::sampler shadowMapSampler,
                              int shadowMapIndex,
                              metal::float3 const projCoords, float NdotL);

float calculateDirLightCascadedShadow(
    metal::texture2d_array<float> shadowMaps, metal::sampler shadowMapSampler,
    int shadowMapOffset, int numCascades,
    metal::float4x4 device const* lightSpaceTransforms,
    metal::float4 worldPosition, float NdotL);

metal::float3 visualizeShadowCascade(
    int shadowMapOffset, int numCascades,
    metal::float4x4 device const* lightSpaceTransforms,
    metal::float4 worldPosition);

} // namespace bloom

#ifndef POPPY_SHADERS_PBRSHADER_H
#define POPPY_SHADERS_PBRSHADER_H

#include <metal_stdlib>

namespace bloom {

struct PBRData {
    metal::float3 albedo;
    float metallic;
    float roughness;
    float ao;
};

metal::float3 PBRLighting(PBRData data, metal::float3 const radiance,
                          metal::float3 const N, metal::float3 const V,
                          metal::float3 const L, metal::float3 const H);

} // namespace bloom

#endif // POPPY_SHADERS_PBRSHADER_H

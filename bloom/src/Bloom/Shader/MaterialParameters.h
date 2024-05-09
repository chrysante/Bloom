#ifndef BLOOM_SHADER_MATERIALPARAMETERS_H
#define BLOOM_SHADER_MATERIALPARAMETERS_H

#include "Shader/Common.h"

namespace bloom {

struct MaterialParameters {
    vml::float3 baseColor;
    float roughness;
    float metallic;
    vml::float3 emissive;
    float emissiveIntensity;
};

} // namespace bloom

#endif // BLOOM_SHADER_MATERIALPARAMETERS_H

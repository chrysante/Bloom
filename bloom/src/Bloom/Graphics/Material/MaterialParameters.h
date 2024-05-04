#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H

#include "Bloom/Shader/Common.h"

namespace bloom {

struct BLOOM_API MaterialParameters {
    vml::float3 baseColor;
    float roughness;
    float metallic;
    vml::float3 emissive;
    float emissiveIntensity;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::MaterialParameters, baseColor, roughness,
                           metallic, emissive, emissiveIntensity);

#endif

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H

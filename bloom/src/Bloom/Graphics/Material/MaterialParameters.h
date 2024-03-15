#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H

#include "Bloom/Graphics/ShaderBase.h"

namespace bloom {

struct BLOOM_API MaterialParameters {
    metal::float3 baseColor;
    float roughness;
    float metallic;
    metal::float3 emissive;
    float emissiveIntensity;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::MaterialParameters, baseColor, roughness,
                           metallic, emissive, emissiveIntensity);

#endif

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H

#ifndef BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H
#define BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H

#include "Bloom/Core/Serialize.h"
#include "Shader/MaterialParameters.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::MaterialParameters, baseColor, roughness,
                           metallic, emissive, emissiveIntensity);

#endif // BLOOM_GRAPHICS_MATERIAL_MATERIALPARAMETERS_H

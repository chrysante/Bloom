#ifndef BLOOM_GRAPHICS_MATERIALPROPERTIES_H
#define BLOOM_GRAPHICS_MATERIALPROPERTIES_H

#include "Bloom/Graphics/ShaderBase.h"

namespace bloom {

struct DefaultLitMaterialProperties {
    float3 color;
    float roughness;
    float metallic;
    float3 normal;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MATERIALPROPERTIES_H

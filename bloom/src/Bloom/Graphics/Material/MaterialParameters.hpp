#include "Bloom/Graphics/ShaderBase.hpp"

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

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::MaterialParameters, baseColor, roughness,
                           metallic, emissive, emissiveIntensity);

#endif

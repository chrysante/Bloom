#ifndef BLOOM_GRAPHICS_LIGHTS_H
#define BLOOM_GRAPHICS_LIGHTS_H

#include "Bloom/Shader/Common.h"

namespace bloom {

struct LightCommon {
    metal::packed_float3 color;
    float intensity;
};

struct PointLight {
    LightCommon common;
    metal::float3 position;
    float radius;

    static PointLight Default();
};

struct SpotLight {
    LightCommon common;
    metal::float3 position;
    metal::float3 direction;
    float innerCutoff;
    float outerCutoff;
    float radius;

    static SpotLight Default();
};

struct DirectionalLight {
    LightCommon common; // 16 bytes

    metal::packed_float3 direction;      //
    bool castsShadows;                   //
    int16_t numCascades;                 // 16 bytes
    float shadowDistance;                //
    float shadowDistanceZ;               //
    float cascadeDistributionExponent;   //
    float cascadeTransitionFraction;     // 16 bytes
    float shadowDistanceFadeoutFraction; //

    static DirectionalLight Default();
};

struct SkyLight {
    LightCommon common;

    static SkyLight Default();
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::LightCommon, color, intensity);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PointLight, common, position, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SpotLight, common, position, direction,
                           innerCutoff, outerCutoff, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::DirectionalLight, common, direction,
                           castsShadows, numCascades, shadowDistance,
                           shadowDistanceZ, cascadeDistributionExponent,
                           cascadeTransitionFraction,
                           shadowDistanceFadeoutFraction);

BLOOM_MAKE_TEXT_SERIALIZER(bloom::SkyLight, common);

#endif

#endif // BLOOM_GRAPHICS_LIGHTS_H

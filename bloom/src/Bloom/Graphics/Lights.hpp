#pragma once

// TODO: Make path relative to base
#include "ShaderBase.hpp"

namespace bloom {

struct LightCommon {
    metal::packed_float3 color;
    float intensity;
};

struct PointLight {
    LightCommon common;
    metal::float3 position;
    float radius;
};

struct SpotLight {
    LightCommon common;
    metal::float3 position;
    metal::float3 direction;
    float innerCutoff;
    float outerCutoff;
    float radius;
};

struct DirectionalLight {
    LightCommon common;                           // 16 bytes

    metal::packed_float3 direction;               //
    bool castsShadows;                            //
    int16_t numCascades = 1;                      // 16 bytes

    float shadowDistance = 500;                   // dedim

    float shadowDistanceZ               = 10'000; //
    float cascadeDistributionExponent   = 2;      //
    float cascadeTransitionFraction     = 0.05;   //
    float shadowDistanceFadeoutFraction = 0.05;   // 16 bytes
};

struct SkyLight {
    LightCommon common;
};

} // namespace bloom

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::LightCommon, color, intensity);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::PointLight, common, position, radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::SpotLight,
                           common,
                           position,
                           direction,
                           innerCutoff,
                           outerCutoff,
                           radius);
BLOOM_MAKE_TEXT_SERIALIZER(bloom::DirectionalLight,
                           common,
                           direction,
                           castsShadows,
                           numCascades,
                           shadowDistance,
                           shadowDistanceZ,
                           cascadeDistributionExponent,
                           cascadeTransitionFraction,
                           shadowDistanceFadeoutFraction);

BLOOM_MAKE_TEXT_SERIALIZER(bloom::SkyLight, common);

#endif

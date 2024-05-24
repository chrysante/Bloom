#ifndef BLOOM_SHADER_LIGHTS_H
#define BLOOM_SHADER_LIGHTS_H

#include "Shader/Common.h"

namespace bloom {

struct LightCommon {
    vml::packed_float3 color;
    float intensity;
};

struct PointLight {
    LightCommon common;
    vml::float3 position;
    float radius;
    float falloff;

    static PointLight Default();
};

struct SpotLight {
    LightCommon common;
    vml::float3 position;
    vml::float3 direction;
    float innerCutoff;
    float outerCutoff;
    float radius;

    static SpotLight Default();
};

struct DirectionalLight {
    LightCommon common; // 16 bytes

    vml::packed_float3 direction;        //
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

#endif // BLOOM_SHADER_LIGHTS_H

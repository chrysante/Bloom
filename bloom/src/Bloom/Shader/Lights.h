#ifndef BLOOM_SHADER_LIGHTS_H
#define BLOOM_SHADER_LIGHTS_H

#include "Shader/Common.h"

namespace bloom {

struct PointLightRenderData {
    vml::packed_float3 color;
    float intensity;
    vml::float3 position;
    float radius;
    float falloff;
};

struct SpotLightRenderData {
    vml::packed_float3 color;
    float intensity;
    vml::float3 position;
    vml::float3 direction;
    float innerCutoff;
    float outerCutoff;
    float radius;
};

struct DirectionalLightRenderData {
    vml::packed_float3 color;
    float intensity;                     // 16 bytes
    vml::packed_float3 direction;        //
    bool castsShadows;                   //
    int16_t numCascades;                 // 16 bytes
    float shadowDistance;                //
    float shadowDistanceZ;               //
    float cascadeDistributionExponent;   //
    float cascadeTransitionFraction;     // 16 bytes
    float shadowDistanceFadeoutFraction; //
};

struct SkyLightRenderData {
    vml::packed_float3 color;
    float intensity;
    vml::packed_float3 SHCoefficients[9];
};

} // namespace bloom

#endif // BLOOM_SHADER_LIGHTS_H

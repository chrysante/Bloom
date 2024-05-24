#ifndef BLOOM_SHADER_SHAREDTYPES_H
#define BLOOM_SHADER_SHAREDTYPES_H

#include "Shader/Common.h"
#include "Shader/Lights.h"

namespace bloom {

///
struct CameraInput {
    vml::float4x4 camera;
    vml::float4x4 inverseCamera;
    vml::float3 position;
    vml::float3 front;
    vml::float2 viewportSize;
};

BLOOM_SHADER_CONSTANT int NumMaxShadowCasters = 32;

///
struct LightBufferContents {
    uint32_t numPointLights;
    PointLightRenderData pointLights[32];

    uint32_t numSpotLights;
    SpotLightRenderData spotLights[32];

    uint32_t numDirLights;
    DirectionalLightRenderData dirLights[32];

    uint32_t numSkyLights;
    SkyLightRenderData skyLights[32];
};

/// Different tone mapping functions
enum class ToneMappingFunction { ACES, Reinhard };

/// Parameter structure for the bloom shaders
struct BloomParameters {
    bool physicallyCorrect = true;
    vml::float3 curve;
    float contribution;
    float intensity;
    float scale;
    float threshold;
    float clamp;
};

/// Parameter structure for the post processing shader
struct PostProcessParameters {
    ToneMappingFunction toneMappingFunction;
};

} // namespace bloom

#endif // BLOOM_SHADER_SHAREDTYPES_H

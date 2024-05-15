#ifndef BLOOM_SHADER_SHAREDTYPES_H
#define BLOOM_SHADER_SHAREDTYPES_H

#include "Shader/Common.h"
#include "Shader/Lights.h"

namespace bloom {

///
struct CameraInput {
    vml::float4x4 camera;
    vml::float4x4 inverseCamera;
    vml::float3 cameraPosition;
};

BLOOM_SHADER_CONSTANT int NumMaxShadowCasters = 32;

///
struct LightBufferContents {
    uint32_t numPointLights;
    PointLight pointLights[32];

    uint32_t numSpotLights;
    SpotLight spotLights[32];

    uint32_t numDirLights;
    DirectionalLight dirLights[32];

    uint32_t numSkyLights;
    SkyLight skyLights[32];

    vml::float3 irradianceCoefficients[9];
};

/// Describes the layout of the shadow map texture array
struct alignas(vml::float4x4) ShadowRenderMetadata {
    /// The number of lights that cast shadows
    int numShadowCasters;

    /// For each shadow caster the number of cascades
    int numCascades[NumMaxShadowCasters];
};

/// Different tone mapping functions
enum class ToneMappingFunction { ACES, reinhard };

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

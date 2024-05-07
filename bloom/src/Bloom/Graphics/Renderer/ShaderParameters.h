#ifndef BLOOM_GRAPHICS_RENDERER_SHADERPARAMETERS_H
#define BLOOM_GRAPHICS_RENDERER_SHADERPARAMETERS_H

#include "Bloom/Graphics/Lights.h"
#include "Bloom/Shader/Common.h"

namespace bloom {

struct LightBufferContents_Depr {
    uint numPointLights;
    PointLight pointLights[32];

    uint numSpotLights;
    SpotLight spotLights[32];

    uint numDirLights;
    DirectionalLight dirLights[32];

    uint numSkyLights;
    SkyLight skyLights[32];
};

BLOOM_SHADER_CONSTANT int maxShadowCascades = 10; // do we need this?
BLOOM_SHADER_CONSTANT int maxShadowCasters = 32;

struct alignas(vml::float4x4) ShadowRenderData {
    int numShadowCasters;
    int numCascades[maxShadowCasters];
};

struct BloomParameters_Depr {
    bool enabled = true;
    bool physicallyCorrect = true;
    vml::float3 curve;
    float contribution;
    float intensity;
    float scale;
    float threshold;
    float clamp;
};

enum class ToneMapping { ACES, reinhard };

struct PostProcessParameters_Depr {
    BloomParameters_Depr bloom;
    ToneMapping tonemapping;
};

struct DebugDrawData {
    float selectionLineWidth;
    bool visualizeShadowCascades;
    vml::float4x4 shadowCascadeVizTransforms[10];
    uint shadowCascadeVizCount;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERER_SHADERPARAMETERS_H

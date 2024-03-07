#pragma once

#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Graphics/ShaderBase.hpp"

namespace bloom {

struct SceneRenderData {
    metal::float4x4 camera;
    metal::float3 cameraPosition;
    metal::float2 screenSize;
    metal::float2 screenResolution;

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

struct alignas(metal::float4x4) ShadowRenderData {
    int numShadowCasters;
    int numCascades[maxShadowCasters];
};

struct BloomParameters {
    bool enabled = true;
    bool physicallyCorrect = true;
    metal::float3 curve;
    float contribution;
    float intensity;
    float scale;
    float threshold;
    float clamp;
};

enum class ToneMapping { ACES, reinhard };

struct PostProcessParameters {
    BloomParameters bloom;
    ToneMapping tonemapping;
};

struct DebugDrawData {
    float selectionLineWidth;
    bool visualizeShadowCascades;
    metal::float4x4 shadowCascadeVizTransforms[10];
    uint shadowCascadeVizCount;
};

struct RendererParameters {
    PostProcessParameters postprocess;
    SceneRenderData scene;
    ShadowRenderData shadowData;
};

} // namespace bloom

#include <metal_stdlib>

#include "Bloom/Graphics/Material/MaterialParameters.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/Vertex.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Lights.h"
#include "PBRShader.h"

using namespace metal;
using namespace bloom;

struct MainPassData {
    float4 positionCS [[position]];
    float3 normalWS;
    float3 positionWS;
    float3 color;
};

vertex MainPassData mainPassVS(SceneRenderParameters device const& scene
                               [[buffer(0)]],
                               Vertex3D device const* vertices [[buffer(1)]],
                               float4x4 device const& transform [[buffer(2)]],
                               uint const vertexID [[vertex_id]])
{
    Vertex3D const v = vertices[vertexID];

    float4 const vertexPositionMS = float4(v.position, 1);
    float4 const vertexNormalMS = float4(v.normal, 0);
    float4 const vertexPositionWS = transform * vertexPositionMS;

    MainPassData result;

    result.positionCS = scene.camera * vertexPositionWS;
    result.normalWS = (transform * vertexNormalMS).xyz;
    result.positionWS = vertexPositionWS.xyz;
    result.color = v.color.rgb;

    return result;
}

struct MainPassFragmentData {
    float4 color [[color(0)]];
};

static float3 computePointLights(PBRData pbr, float3 N, float3 V,
                                 float3 worldPosition, uint numLights,
                                 PointLight device const* lights)
{
    float3 lightAcc{};
    for (uint i = 0; i < numLights; ++i) {
        lightAcc += calculatePointLight(pbr, lights[i], V, N, worldPosition);
    }
    return lightAcc;
}

static float3 computeSpotLights(PBRData pbr, float3 N, float3 V,
                                float3 worldPosition, uint numLights,
                                SpotLight device const* lights)
{
    float3 lightAcc{};
    for (uint i = 0; i < numLights; ++i) {
        lightAcc += calculateSpotlight(pbr, lights[i], V, N, worldPosition);
    }
    return lightAcc;
}

static float3 computeDirLights(PBRData pbr, float3 N, float3 V,
                               float3 worldPosition,
                               float4x4 const device* lightSpaceTransforms,
                               uint numLights,
                               DirectionalLight device const* lights,
                               bloom::ShadowRenderData const device& shadowData,
                               sampler shadowMapSampler,
                               texture2d_array<float> shadowMaps)
{
    float3 lightAcc{};
    for (uint i = 0, shadowCasterIndex = 0, shadowMapOffset = 0; i < numLights;
         ++i)
    {
        auto light = lights[i];
        float3 dirLight = calculateDirectionalLight(pbr, light, V, N);
        if (!light.castsShadows) {
            lightAcc += dirLight;
            continue;
        }
        int numCascades = shadowData.numCascades[shadowCasterIndex];
        float shadow =
            calculateDirLightCascadedShadow(shadowMaps, shadowMapSampler,
                                            shadowMapOffset, numCascades,
                                            lightSpaceTransforms,
                                            float4(worldPosition, 1),
                                            dot(N, light.direction));
        shadowMapOffset += numCascades;
        ++shadowCasterIndex;
        lightAcc += shadow * dirLight;
    }
    return lightAcc;
}

static float3 computeSkyLights(PBRData pbr, uint numLights,
                               SkyLight device const* lights)
{
    float3 lightAcc{};
    for (uint i = 0; i < numLights; ++i) {
        SkyLight light = lights[i];
        lightAcc +=
            pbr.albedo * pbr.ao * light.common.color * light.common.intensity;
    }
    return lightAcc;
}

static float3 computeEmissive(MaterialParameters materialParams)
{
    return materialParams.emissive * materialParams.emissiveIntensity;
}

fragment MainPassFragmentData
    mainPassFS(MainPassData in [[stage_in]],
               RendererParameters device const& params [[buffer(0)]],
               float4x4 device const* lightSpaceTransforms [[buffer(1)]],
               MaterialParameters device const& materialParams [[buffer(2)]],
               texture2d_array<float> shadowMaps [[texture(0)]],
               sampler shadowMapSampler [[sampler(0)]])
{
    SceneRenderParameters device const& scene = params.scene;
    ShadowRenderData device const& shadowData = params.shadowData;
    PBRData pbr{ .albedo = materialParams.baseColor,
                 .metallic = materialParams.metallic,
                 .roughness = materialParams.roughness,
                 .ao = 1 };
    float3 worldPosition = in.positionWS;
    float3 N = normalize(in.normalWS.xyz);
    float3 V = normalize(scene.cameraPosition - worldPosition);
    float3 lightAcc = 0;
    lightAcc += computePointLights(pbr, N, V, worldPosition,
                                   scene.lights.numPointLights,
                                   scene.lights.pointLights);
    lightAcc += computeSpotLights(pbr, N, V, worldPosition,
                                  scene.lights.numSpotLights,
                                  scene.lights.spotLights);
    lightAcc += computeDirLights(pbr, N, V, worldPosition, lightSpaceTransforms,
                                 scene.lights.numDirLights,
                                 scene.lights.dirLights, shadowData,
                                 shadowMapSampler, shadowMaps);
    lightAcc += computeSkyLights(pbr, scene.lights.numSkyLights,
                                 scene.lights.skyLights);
    lightAcc += computeEmissive(materialParams);
    return MainPassFragmentData{ .color = float4(lightAcc, 1) };
}

// # Render graph shaders

vertex MainPassData rgMainPassVS(CameraInput device const& cameraBuffer
                                 [[buffer(0)]],
                                 Vertex3D device const* vertices [[buffer(1)]],
                                 float4x4 device const& transform [[buffer(2)]],
                                 unsigned vertexID [[vertex_id]])
{
    Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 vertexNormalMS = float4(v.normal, 0);
    float4 vertexPositionWS = transform * vertexPositionMS;
    MainPassData result;
    result.positionCS = cameraBuffer.camera * vertexPositionWS;
    result.normalWS = (transform * vertexNormalMS).xyz;
    result.positionWS = vertexPositionWS.xyz;
    result.color = v.color.rgb;
    return result;
}

fragment MainPassFragmentData
    rgMainPassFS(MainPassData in [[stage_in]],
                 CameraInput device const& cameraBuffer [[buffer(0)]],
                 LightBufferContents device const& lights [[buffer(1)]],
                 float4x4 device const* lightSpaceTransforms [[buffer(2)]],
                 MaterialParameters device const& materialParams [[buffer(3)]],
                 texture2d_array<float> shadowMaps [[texture(0)]],
                 sampler shadowMapSampler [[sampler(0)]])
{
    PBRData pbr{ .albedo = materialParams.baseColor,
                 .metallic = materialParams.metallic,
                 .roughness = materialParams.roughness,
                 .ao = 1 };
    float3 worldPosition = in.positionWS;
    float3 N = normalize(in.normalWS.xyz);
    float3 V = normalize(cameraBuffer.cameraPosition - worldPosition);
    float3 lightAcc = 0;
    lightAcc += computePointLights(pbr, N, V, worldPosition,
                                   lights.numPointLights, lights.pointLights);
    lightAcc += computeSpotLights(pbr, N, V, worldPosition,
                                  lights.numSpotLights, lights.spotLights);
#if 0
    lightAcc += computeDirLights(pbr, N, V, worldPosition, lightSpaceTransforms,
                                 lights.numDirLights, lights.dirLights,
                                 shadowData, shadowMapSampler, shadowMaps);
#endif
    lightAcc += computeSkyLights(pbr, lights.numSkyLights, lights.skyLights);
    lightAcc += computeEmissive(materialParams);
    return MainPassFragmentData{ .color = float4(lightAcc, 1) };
}

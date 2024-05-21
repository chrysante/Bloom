#include "Shader/PBRShader.h"

#include <metal_stdlib>

using namespace metal;
using namespace bloom;

static float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

static float distributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    float PI = 3.14159265359;
    denom = PI * denom * denom;
    return num / denom;
}

static float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return num / denom;
}

static float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

static float3 computeF0(float3 albedo, float3 metallic)
{
    float3 F0_dielectric = float3(0.04);
    return mix(F0_dielectric, albedo, metallic);
}

static float3 PBRLighting(PBRData data, float3 const radiance, float3 const N,
                          float3 const V, float3 const L, float3 const H)
{
    float3 F0 = computeF0(data.albedo, data.metallic);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = distributionGGX(N, H, data.roughness);
    float G = geometrySmith(N, V, L, data.roughness);
    float3 numerator = NDF * G * F;
    float denominator =
        4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - data.metallic);
    float PI = 3.14159265359;
    float NdotL = max(dot(N, L), 0.0);
    return (kD * data.albedo / PI + specular) * radiance * NdotL;
}

static float3 calculatePointLight(PBRData pbrData, PointLight light, float3 V,
                                  float3 N, float3 worldPosition)
{
    float3 L = normalize(light.position - worldPosition);
    float3 H = normalize(V + L);
    float dist =
        max(length(light.position - worldPosition) - light.radius, 0.0001);
    float attenuation = 1.0 / (dist * dist);
    float3 radiance = light.common.color * light.common.intensity * attenuation;
    return PBRLighting(pbrData, radiance, N, V, L, H);
}

static float3 calculateSpotlight(PBRData pbrData, SpotLight light, float3 V,
                                 float3 N, float3 worldPosition)
{
    float3 L = normalize(light.position - worldPosition);
    float theta = dot(L, -light.direction);
    float epsilon = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    if (theta == 0) {
        return 0;
    }
    float3 H = normalize(V + L);
    float dist =
        max(length(light.position - worldPosition) - light.radius, 0.0001);
    float attenuation = 1.0 / (dist * dist);
    float3 radiance =
        intensity * light.common.color * light.common.intensity * attenuation;
    return PBRLighting(pbrData, radiance, N, V, L, H);
}

static float3 calculateDirectionalLight(PBRData pbrData, DirectionalLight light,
                                        float3 V, float3 N)
{
    float3 L = light.direction;
    float3 H = normalize(V + L);
    float3 radiance = light.common.color * light.common.intensity;
    return PBRLighting(pbrData, radiance, N, V, L, H);
}

static bool isInLightFrustum(float3 const projCoords)
{
    bool3 result = projCoords >= 0 && projCoords <= 1;
    return result.x && result.y && result.z;
}

static float3 calculateLightProjection(float4 const positionLightSpace)
{
    /// Perform perspective divide
    float3 projCoords = positionLightSpace.xyz / positionLightSpace.w;
    projCoords.y *= -1;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    return projCoords;
}

static float calculateDirLightShadow(texture2d_array<float> shadowMap,
                                     int shadowMapIndex,
                                     float3 const projCoords, float NdotL)
{
    if (!isInLightFrustum(projCoords)) {
        return 1.0;
    }
    constexpr sampler ShadowMapSampler;
    float closestDepth =
        shadowMap.sample(ShadowMapSampler, projCoords.xy, shadowMapIndex).r;
    /// Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    float shadow = currentDepth > closestDepth ? 0.0 : 1.0;
    return shadow;
}

static float calculateDirLightCascadedShadow(
    texture2d_array<float> shadowMaps, int shadowMapOffset, int numCascades,
    float4x4 device const* lightSpaceTransforms, float4 worldPosition,
    float NdotL)
{
    float shadow = 1;
    for (int j = numCascades - 1; j >= 0; --j) {
        int shadowMapIndex = shadowMapOffset + j;
        float4 positionLightSpace =
            lightSpaceTransforms[shadowMapIndex] * worldPosition;
        float3 projLightCoords = calculateLightProjection(positionLightSpace);
        bool isInFrustum = isInLightFrustum(projLightCoords);
        if (isInFrustum) {
            shadow = calculateDirLightShadow(shadowMaps, shadowMapIndex,
                                             projLightCoords, NdotL);
        }
    }
    return shadow;
}

constant float3 shadowVizColors[10] = {
    float3(1.0, 0.0, 0.0), float3(1.0, 0.5, 0.0), float3(1.0, 1.0, 0.0),
    float3(0.5, 1.0, 0.0), float3(0.0, 1.0, 0.5), float3(0.0, 1.0, 1.0),
    float3(0.0, 0.5, 1.0), float3(0.0, 0.0, 1.0), float3(0.5, 0.0, 1.0),
    float3(1.0, 0.0, 1.0)
};

[[maybe_unused]] static float3 visualizeShadowCascade(
    int shadowMapOffset, int numCascades,
    float4x4 device const* lightSpaceTransforms, float4 worldPosition)
{
    float3 shadowViz = 0.5;
    for (int j = numCascades - 1; j >= 0; --j) {
        int lightMapIndex = shadowMapOffset + j;
        float4 positionLightSpace =
            lightSpaceTransforms[lightMapIndex] * worldPosition;
        float3 projLightCoords = calculateLightProjection(positionLightSpace);
        bool isInFrustum = isInLightFrustum(projLightCoords);
        if (isInFrustum) {
            shadowViz = shadowVizColors[j];
        }
    }
    return shadowViz;
}

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
                               ShadowRenderMetadata const device& shadowData,
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
            calculateDirLightCascadedShadow(shadowMaps, shadowMapOffset,
                                            numCascades, lightSpaceTransforms,
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

static float3 calcSHIrradiance(float3 N, float3 device const* SH)
{
    float A0 = 3.141593;
    float A1 = 2.094395;
    float A2 = 0.785398;
    float3 color = SH[0] * 0.282095f * A0 + SH[1] * 0.488603f * N.y * A1 +
                   SH[2] * 0.488603f * N.z * A1 + SH[3] * 0.488603f * N.x * A1 +
                   SH[4] * 1.092548f * N.x * N.y * A2 +
                   SH[5] * 1.092548f * N.y * N.z * A2 +
                   SH[6] * 0.315392f * (3.0f * N.z * N.z - 1.0f) * A2 +
                   SH[7] * 1.092548f * N.x * N.z * A2 +
                   SH[8] * 0.546274f * (N.x * N.x - N.y * N.y) * A2;
    float PI = 3.14159265359;
    return color / PI;
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness), F0) - F0) *
                    pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

static float3 computeAmbient(PBRData data, float3 N, float3 V,
                             float3 device const* SH,
                             texturecube<float> specularMap,
                             texture2d<float> brdfMap)
{
    float3 irradiance = calcSHIrradiance(N, SH);
    float3 diffuse = irradiance * data.albedo;
    float3 F0 = computeF0(data.albedo, data.metallic);
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, data.roughness);
    float3 kD = (1.0f - F) * (1.0f - data.metallic);
    float3 R = reflect(-V, N);
    float mipLevel = data.roughness * (specularMap.get_num_mip_levels() - 1);
    constexpr sampler specSampler(min_filter::linear, mag_filter::linear,
                                  mip_filter::linear);
    float3 prefilteredColor =
        specularMap.sample(specSampler, R, level(mipLevel)).rgb;
    constexpr sampler brdfSampler(min_filter::linear, mag_filter::linear);
    float2 brdfUV = { max(dot(N, V), 0.0), data.roughness };
    float2 envBRDF = brdfMap.sample(brdfSampler, brdfUV).rg;
    // TODO: envBRDF should not be NAN. This must be fixed when generating the
    // texture
    if (isnan(envBRDF.x)) {
        envBRDF.x = 0;
    }
    if (isnan(envBRDF.y)) {
        envBRDF.y = 0;
    }
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    return (kD * diffuse + specular) * data.ao;
}

float3 bloom::PBRForwardShader(
    PBRData pbr, float3 worldPosition, float3 cameraPosition,
    device const float4x4* lightSpaceTransforms,
    device const LightBufferContents& lights,
    device const ShadowRenderMetadata& shadowMetadata,
    texture2d_array<float> shadowMaps, texturecube<float> specularMap,
    texture2d<float> brdfMap)
{
    float3 N = pbr.normal;
    float3 V = normalize(cameraPosition - worldPosition);
    float3 lightAcc = 0;
    lightAcc += computePointLights(pbr, N, V, worldPosition,
                                   lights.numPointLights, lights.pointLights);
    lightAcc += computeSpotLights(pbr, N, V, worldPosition,
                                  lights.numSpotLights, lights.spotLights);
    lightAcc += computeDirLights(pbr, N, V, worldPosition, lightSpaceTransforms,
                                 lights.numDirLights, lights.dirLights,
                                 shadowMetadata, shadowMaps);
    lightAcc += computeSkyLights(pbr, lights.numSkyLights, lights.skyLights);
    lightAcc += computeAmbient(pbr, N, V, lights.irradianceCoefficients,
                               specularMap, brdfMap);
    lightAcc += pbr.emissive;
    return lightAcc;
}

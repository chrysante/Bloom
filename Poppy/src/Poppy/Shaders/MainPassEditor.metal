#include <metal_stdlib>
#include "Bloom/Graphics/SceneRenderData.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "Bloom/Graphics/Vertex.hpp"
#include "Bloom/Graphics/EntityRenderData.hpp"

#include "PBRShader.h"

using namespace metal;
using namespace bloom;

struct MainPassData {
	float4 positionCS [[ position ]];
	
	float3 normalWS;
	float3 positionWS;
	float3 color;
	
	uint entityID;
};

static float3 calculateLightProjection(float4 const positionLightSpace) {
	float3 projCoords = positionLightSpace.xyz / positionLightSpace.w;
	projCoords.y *= -1;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	return projCoords;
}

vertex MainPassData mainPassEditorVS(Vertex3D device const* vertices [[ buffer(1) ]],
									 uint const vertexID [[ vertex_id ]],
									 SceneRenderData device const& scene [[ buffer(0) ]],
									 ShadowRenderData device const* shadows [[ buffer(3) ]],
									 EntityRenderData device const& entity [[ buffer(2) ]])
{
	Vertex3D const v = vertices[vertexID];
	float4 const vertexPositionMS = float4(v.position, 1);
	float4 const vertexNormalMS = float4(v.normal, 0);

	float4 const vertexPositionWS = entity.transform * vertexPositionMS;
	
	MainPassData result;
	
	result.positionCS = scene.camera * vertexPositionWS;
	result.normalWS = (entity.transform * vertexNormalMS).xyz;
	result.positionWS = vertexPositionWS.xyz;
	result.color = v.color.rgb;
	result.entityID = entity.ID;
	
	return result;
}

struct FragmentData {
	float4 color  [[ color(0) ]];
	uint entityID [[ color(1) ]];
	float4 shadowCascade [[ color(3) ]];
};

static bool isInLightFrustum(float3 const projCoords) {
	bool3 const result = projCoords >= 0 && projCoords <= 1;
	return result.x && result.y && result.z;
}

static float calculateShadowDirLight(float3 const projCoords,
									 texture2d_array<float> shadowMap,
									 sampler shadowMapSampler,
									 int shadowMapIndex) {
	// perform perspective divide
	if (!isInLightFrustum(projCoords)) {
		return 1.0;
	}
	
	float const closestDepth = shadowMap.sample(shadowMapSampler,
												projCoords.xy,
												shadowMapIndex).r;
	
	// get depth of current fragment from light's perspective
	float const currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float const bias = 0.005;
	float const shadow = currentDepth - bias > closestDepth  ? 0.0 : 1.0;

	return shadow;
}


constant float3 shadowVizColors[10] = {
	float3(1.0, 0.0, 0.0),
	float3(1.0, 0.5, 0.0),
	float3(1.0, 1.0, 0.0),
	float3(0.5, 1.0, 0.0),
	float3(0.0, 1.0, 0.5),
	float3(0.0, 1.0, 1.0),
	float3(0.0, 0.5, 1.0),
	float3(0.0, 0.0, 1.0),
	float3(0.5, 0.0, 1.0),
	float3(1.0, 0.0, 1.0)
};

fragment FragmentData mainPassEditorFS(MainPassData in                             [[ stage_in ]],
									   SceneRenderData device const& scene         [[ buffer(0) ]],
									   DebugDrawData device const& debugData       [[ buffer(1) ]],
									   ShadowRenderData device const& shadowData   [[ buffer(2) ]],
									   float4x4 device const* lightSpaceTransforms [[ buffer(3) ]],
									   texture2d_array<float> shadowMaps           [[ texture(0) ]],
									   sampler shadowMapSampler                    [[ sampler(0) ]])
{
	FragmentData result;
	result.entityID = in.entityID;
	
	// Lighting
	float3 const worldPosition = in.positionWS;
	float3 const N = normalize(in.normalWS.xyz);
	float3 const V = normalize(scene.cameraPosition - worldPosition);
	
	PBRData const data {
		.albedo    = float3(.6, .9, .2),
		.metallic  = 0,
		.roughness = 0.5,
		.ao        = 1
	};
	
	float3 lightAcc = 0;
	
	// Point Lights
	for (uint i = 0; i < scene.numPointLights; ++i) {
		float3 const lightPosition = scene.pointLights[i].position;
		PointLight const light = scene.pointLights[i].light;
		
		float3 const L = normalize(lightPosition - worldPosition);
		float3 const H = normalize(V + L);
		
		float const dist = max(length(lightPosition - worldPosition) - light.radius, 0.0001);
		float const attenuation = 1.0 / (dist * dist);
		
		float3 const radiance = light.common.color * light.common.intensity * attenuation;
		
		lightAcc += PBRLighting(data, radiance, N, V, L, H);
	}
	
	// Spotlights
	for (uint i = 0; i < scene.numSpotLights; ++i) {
		float3 const lightPosition = scene.spotLights[i].position;
		float3 const lightDirection = scene.spotLights[i].direction;
		SpotLight const light = scene.spotLights[i].light;
		
		float3 const L = normalize(lightPosition - worldPosition);
		
		float const theta     = dot(L, -lightDirection);
		float const epsilon   = light.innerCutoff - light.outerCutoff;
		float const intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
		
		if (theta == 0) {
			continue;
		}
		
		float3 const H = normalize(V + L);
		
		float const dist = max(length(lightPosition - worldPosition) - light.radius, 0.0001);
		float const attenuation = 1.0 / (dist * dist);
		
		float3 const radiance = intensity * light.common.color * light.common.intensity * attenuation;
		
		lightAcc += PBRLighting(data, radiance, N, V, L, H);
	}
	
	// Directional Lights
	for (uint i = 0, shadowCasterIndex = 0, shadowMapOffset = 0; i < scene.numDirLights; ++i) {
		float3 const lightDirection = scene.dirLights[i].direction;
		auto const light = scene.dirLights[i];
		
		float3 const L = lightDirection;
		float3 const H = normalize(V + L);
		float3 const radiance = light.light.common.color * light.light.common.intensity;
		
		float3 const dirLight = PBRLighting(data, radiance, N, V, L, H);
		
		
		float shadow = 1;
		
		if (light.light.castsShadows) {
			int const numCascades = shadowData.numCascades[shadowCasterIndex];
			for (int j = numCascades - 1; j >= 0; --j) {
				int const shadowMapIndex = shadowMapOffset + j;
				float4 const positionLightSpace = lightSpaceTransforms[shadowMapIndex] * float4(in.positionWS, 1);
				float3 const projLightCoords = calculateLightProjection(positionLightSpace);
				bool const isInFrustum = isInLightFrustum(projLightCoords);
				if (isInFrustum) {
					shadow = calculateShadowDirLight(projLightCoords,
													 shadowMaps,
													 shadowMapSampler,
													 shadowMapIndex);
				}
			}
			shadowMapOffset += numCascades;
			++shadowCasterIndex;
		}
		
		lightAcc += dirLight * shadow;
	}
	
	
	// visualize shadow cascade
	if (debugData.visualizeShadowCascades) {
		float3 shadowViz = 0.5;
		for (int j = shadowData.numCascades[debugData.shadowCasterIndex] - 1; j >= 0; --j) {
			int const lightMapIndex = debugData.shadowMapOffset + j;
			
			float4 const positionLightSpace = lightSpaceTransforms[lightMapIndex] * float4(in.positionWS, 1);
			float3 const projLightCoords = calculateLightProjection(positionLightSpace);
			bool const isInFrustum = isInLightFrustum(projLightCoords);
			if (isInFrustum) {
				shadowViz = shadowVizColors[j];
			}
		}
		result.shadowCascade = float4(shadowViz, 1);
	}
	
	float3 const ambient    = 0.03 * data.albedo * data.ao;
	float3 const finalColor = ambient + lightAcc;
	
	result.color = float4(finalColor, 1);
	
	return result;
}


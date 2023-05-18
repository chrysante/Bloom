#include "Lights.h"

using namespace metal;

namespace bloom {
	
    /// **Lights**
    
	float3 calculatePointLight(PBRData pbrData, PointLight light, float3 V, float3 N, float3 worldPosition) {
		float3 const L = normalize(light.position - worldPosition);
		float3 const H = normalize(V + L);
		
		float const dist = max(length(light.position - worldPosition) - light.radius, 0.0001);
		float const attenuation = 1.0 / (dist * dist);
		
		float3 const radiance = light.common.color * light.common.intensity * attenuation;
		
		return PBRLighting(pbrData, radiance, N, V, L, H);
	}

	float3 calculateSpotlight(PBRData pbrData, SpotLight light, float3 V, float3 N, float3 worldPosition) {
		float3 const L = normalize(light.position - worldPosition);
		
		float const theta     = dot(L, -light.direction);
		float const epsilon   = light.innerCutoff - light.outerCutoff;
		float const intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
		
		if (theta == 0) {
			return 0;
		}
		
		float3 const H = normalize(V + L);
		
		float const dist = max(length(light.position - worldPosition) - light.radius, 0.0001);
		float const attenuation = 1.0 / (dist * dist);
		
		float3 const radiance = intensity * light.common.color * light.common.intensity * attenuation;
		
		return PBRLighting(pbrData, radiance, N, V, L, H);
	}

	float3 calculateDirectionalLight(PBRData pbrData, DirectionalLight light, float3 V, float3 N) {
		float3 const L = light.direction;
		float3 const H = normalize(V + L);
		float3 const radiance = light.common.color * light.common.intensity;

		return PBRLighting(pbrData, radiance, N, V, L, H);
	}
	
	/// ** Shadows **

    static bool isInLightFrustum(float3 const projCoords) {
		bool3 const result = projCoords >= 0 && projCoords <= 1;
		return result.x && result.y && result.z;
	}

	static float3 calculateLightProjection(float4 const positionLightSpace) {
		/// Perform perspective divide
		float3 projCoords = positionLightSpace.xyz / positionLightSpace.w;
		projCoords.y *= -1;
		projCoords.xy = projCoords.xy * 0.5 + 0.5;
		return projCoords;
	}
	
	float calculateDirLightShadow(texture2d_array<float> shadowMap,
								  sampler shadowMapSampler,
								  int shadowMapIndex,
								  float3 const projCoords,
								  float NdotL)
	{
		if (!isInLightFrustum(projCoords)) {
			return 1.0;
		}
		float const closestDepth = shadowMap.sample(shadowMapSampler,
													projCoords.xy,
													shadowMapIndex).r;
		/// Get depth of current fragment from light's perspective
		float const currentDepth = projCoords.z;
		float shadow = currentDepth > closestDepth  ? 0.0 : 1.0;
		return shadow;
	}

	float calculateDirLightCascadedShadow(texture2d_array<float> shadowMaps,
										  sampler shadowMapSampler,
										  int shadowMapOffset,
										  int numCascades,
										  float4x4 device const* lightSpaceTransforms,
										  float4 worldPosition,
										  float NdotL)
	{
		float shadow = 1;
		for (int j = numCascades - 1; j >= 0; --j) {
			int const shadowMapIndex = shadowMapOffset + j;
			float4 const positionLightSpace = lightSpaceTransforms[shadowMapIndex] * worldPosition;
			float3 const projLightCoords = calculateLightProjection(positionLightSpace);
			bool const isInFrustum = isInLightFrustum(projLightCoords);
			if (isInFrustum) {
				shadow = calculateDirLightShadow(shadowMaps,
												 shadowMapSampler,
												 shadowMapIndex,
												 projLightCoords,
												 NdotL);
			}
		}
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
	
	float3 visualizeShadowCascade(int shadowMapOffset,
								  int numCascades,
								  float4x4 device const* lightSpaceTransforms,
								  float4 worldPosition)
	{
		float3 shadowViz = 0.5;
		for (int j = numCascades - 1; j >= 0; --j) {
			int const lightMapIndex = shadowMapOffset + j;
			float4 const positionLightSpace = lightSpaceTransforms[lightMapIndex] * worldPosition;
			float3 const projLightCoords = calculateLightProjection(positionLightSpace);
			bool const isInFrustum = isInLightFrustum(projLightCoords);
			if (isInFrustum) {
				shadowViz = shadowVizColors[j];
			}
		}
		return shadowViz;
	}
}

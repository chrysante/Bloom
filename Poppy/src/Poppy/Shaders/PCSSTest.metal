#include <metal_stdlib>
#include "Bloom/Graphics/SceneRenderData.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "Bloom/Graphics/Vertex.hpp"
#include "Bloom/Graphics/EntityRenderData.hpp"

using namespace metal;
using namespace bloom;



	

struct MainPassData {
	float4 positionCS [[ position ]];
	
	float3 normalWS;
	float3 positionWS;
	float3 color;
};


static bool isInLightFrustum(float3 const projCoords) {
	bool3 const result = projCoords >= 0 && projCoords <= 1;
	return result.x && result.y && result.z;
}

static float3 calculateLightProjection(float4 const positionLightSpace) {
	// perform perspective divide
	float3 projCoords = positionLightSpace.xyz / positionLightSpace.w;
	projCoords.y *= -1;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;
	return projCoords;
}


static float calculateSearchWidth(float uvLightSize,
								  float receiverDistance,
								  float nearPlane,
								  float3 cameraPosition)
{
	return uvLightSize * (receiverDistance - nearPlane) / cameraPosition.z;
}

static float randomFloat(float seed) {
	return fract(sin(seed) * 10000000);
}


static float2 randomDirection(float seed) {
	return normalize(float2(randomFloat(seed), randomFloat(seed + 104734.546)));
}

static float findBlockerDistance(float3 shadowCoords,
								 float uvLightSize,
								 texture2d_array<float> shadowMaps,
								 sampler shadowMapSampler,
								 int shadowMapIndex,
								 float3 cameraPosition)
{
	int blockers = 0;
	float avgBlockerDistance = 0;
	float const nearPlane = 1;
	float searchWidth = calculateSearchWidth(uvLightSize, shadowCoords.z, nearPlane, cameraPosition);
	
	float const directionalLightShadowMapBias = 0;
	
	int const numBlockerSearchSamples = 32;
	
	for (int i = 0; i < numBlockerSearchSamples; i++)
	{
		float z = shadowMaps.sample(shadowMapSampler, shadowCoords.xy + randomDirection(i / float(numBlockerSearchSamples)) * searchWidth, shadowMapIndex).r;
//		return shadowCoords.z ;
//		return z;
		if (z < (shadowCoords.z - directionalLightShadowMapBias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}
	
	return float(blockers) / numBlockerSearchSamples;
	
	if (blockers > 0)
		return avgBlockerDistance / blockers;
	else
		return 0.0;
}




fragment float4 mainPassFS_PCSSTest(MainPassData                   in                   [[ stage_in   ]],
										 SceneRenderData device const&  scene                [[ buffer(0)  ]],
										 ShadowRenderData device const& shadowData           [[ buffer(1)  ]],
										 float4x4 device const*         lightSpaceTransforms [[ buffer(2)  ]],
										 texture2d_array<float>         shadowMaps           [[ texture(0) ]],
										 sampler                        shadowMapSampler     [[ sampler(0) ]])
{
	
	float3 const shadowCoords = calculateLightProjection(lightSpaceTransforms[0] * float4(in.positionWS, 1));

	float const uvLightSize = .7;
	float3 const cameraPosition = scene.cameraPosition;

	int const shadowMapIndex = 0;
	float const blockerDistance = findBlockerDistance(shadowCoords,
													  uvLightSize,
													  shadowMaps,
													  shadowMapSampler,
													  shadowMapIndex,
													  cameraPosition);





	return float4(float3(blockerDistance), 1);
	
//	float3 const projCoords = calculateLightProjection(lightSpaceTransforms[0] * float4(in.positionWS, 1));
//
//	float const closestDepth = shadowMaps.sample(shadowMapSampler,
//											  projCoords.xy,
//											  0).r;
//	
//	// get depth of current fragment from light's perspective
//	float const currentDepth = projCoords.z;
//	
//	float shadow = currentDepth > closestDepth  ? 0.0 : 1.0;
//
//	return closestDepth;
//	return float4(float3(shadow), 1);
}




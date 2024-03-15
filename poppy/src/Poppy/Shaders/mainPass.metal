#include <metal_stdlib>
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/Material/MaterialParameters.h"
#include "Bloom/Graphics/MaterialProperties.h" // not really useful
#include "Bloom/Graphics/Vertex.h"

#include "PBRShader.h"
#include "Lights.h"

using namespace metal;
using namespace bloom;

struct MainPassData {
	float4 positionCS [[ position ]];
	
	float3 normalWS;
	float3 positionWS;
	float3 color;
};

vertex MainPassData mainPassVS(SceneRenderData device const&  scene     [[ buffer(0) ]],
							   Vertex3D device const*         vertices  [[ buffer(1) ]],
							   float4x4 device const&         transform [[ buffer(2) ]],
							   uint const                     vertexID  [[ vertex_id ]])
{
	Vertex3D const v = vertices[vertexID];
	
	float4 const vertexPositionMS = float4(v.position, 1);
	float4 const vertexNormalMS   = float4(v.normal, 0);
	float4 const vertexPositionWS = transform * vertexPositionMS;
	
	MainPassData result;
	
	result.positionCS = scene.camera * vertexPositionWS;
	result.normalWS   = (transform * vertexNormalMS).xyz;
	result.positionWS = vertexPositionWS.xyz;
	result.color      = v.color.rgb;
	
	return result;
}

struct MainPassFragmentData {
	float4 color [[ color(0) ]];
};

fragment MainPassFragmentData mainPassFS(MainPassData                     in                   [[ stage_in   ]],
										 RendererParameters device const& params               [[ buffer(0)  ]],
										 float4x4 device const*           lightSpaceTransforms [[ buffer(1)  ]],
										 MaterialParameters device const& materialParams       [[ buffer(2)  ]],
										 texture2d_array<float>           shadowMaps           [[ texture(0) ]],
										 sampler                          shadowMapSampler     [[ sampler(0) ]])
{
	SceneRenderData device const&  scene = params.scene;
	ShadowRenderData device const& shadowData = params.shadowData;
	
	// Material Properties
	PBRData const data {
		.albedo    = materialParams.baseColor,
		.metallic  = materialParams.metallic,
		.roughness = materialParams.roughness,
		.ao        = 1
	};


	// Lighting
	float3 const worldPosition = in.positionWS;
	float3 const N = normalize(in.normalWS.xyz);
	float3 const V = normalize(scene.cameraPosition - worldPosition);

	float3 lightAcc = 0;

	// Point Lights
	for (uint i = 0; i < scene.numPointLights; ++i) {
		PointLight const light = scene.pointLights[i];
		lightAcc += calculatePointLight(data, light, V, N, worldPosition);
	}

	// Spotlights
	for (uint i = 0; i < scene.numSpotLights; ++i) {
		SpotLight const light = scene.spotLights[i];
		lightAcc += calculateSpotlight(data, light, V, N, worldPosition);
	}

	// Directional Lights
	for (uint i = 0, shadowCasterIndex = 0, shadowMapOffset = 0; i < scene.numDirLights; ++i) {
		auto const light = scene.dirLights[i];
		float3 const dirLight = calculateDirectionalLight(data, light, V, N);

		if (!light.castsShadows) {
			lightAcc += dirLight;
		}
		else {
			int const numCascades = shadowData.numCascades[shadowCasterIndex];
			float const shadow = calculateDirLightCascadedShadow(shadowMaps,
																 shadowMapSampler,
																 shadowMapOffset,
																 numCascades,
																 lightSpaceTransforms,
																 float4(in.positionWS, 1),
																 dot(N, light.direction));
			shadowMapOffset += numCascades;
			++shadowCasterIndex;

			lightAcc += shadow * dirLight;
		}
	}

	// Sky Lights
	for (uint i = 0; i < scene.numSkyLights; ++i) {
		SkyLight const light = scene.skyLights[i];
		lightAcc += data.albedo * data.ao * light.common.color * light.common.intensity;
	}
	
	// Emissive
	lightAcc += materialParams.emissive * materialParams.emissiveIntensity;
	
	MainPassFragmentData result;

	result.color = float4(lightAcc, 1);

	return result;
}


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
	
	uint entityID;
};

vertex MainPassData mainPassEditorVS(Vertex3D device const* vertices [[ buffer(1) ]],
									 uint const vertexID [[ vertex_id ]],
									 SceneRenderData device const& scene [[ buffer(0) ]],
									 EntityRenderData device const& entity [[ buffer(2) ]])
{
	Vertex3D const v = vertices[vertexID];
	float4 const vertexPositionMS = float4(v.position, 1);
	float4 const vertexNormalMS = float4(v.normal, 0);

	float4 const vertexPositionWS = entity.transform * vertexPositionMS;
	
	return {
		.positionCS = scene.camera * vertexPositionWS,
		.normalWS = (entity.transform * vertexNormalMS).xyz,
		.positionWS = vertexPositionWS.xyz,
		.color = v.color.rgb,
		.entityID = entity.ID
	};
}

struct FragmentData {
	float4 color [[ color(0) ]];
	uint entityID [[ color(1) ]];
};


struct PBRData {
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
};

/*
 float const dist        = length(lightPosition - in.positionWS.xyz);
 float const attenuation = 1.0 / (light.constantTerm + light.linearTerm * dist + light.quadraticTerm * dist * dist);
 finalColor += attenuation;
 */


static float3 fresnelSchlick(float cosTheta, float3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

static float distributionGGX(float3 N, float3 H, float roughness) {
	float const a      = roughness * roughness;
	float const a2     = a * a;
	float const NdotH  = max(dot(N, H), 0.0);
	float const NdotH2 = NdotH * NdotH;
	
	float const num   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	
	float const PI = 3.14159265359;
	
	denom = PI * denom * denom;
	
	return num / denom;
}

static float geometrySchlickGGX(float NdotV, float roughness) {
	float r = roughness + 1.0;
	float k = r * r / 8.0;

	float num   = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	
	return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2  = geometrySchlickGGX(NdotV, roughness);
	float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
	return ggx1 * ggx2;
}


static float3 PBRLighting(PBRData data,
						  float3 const radiance,
						  float3 const N,
						  float3 const V,
						  float3 const L,
						  float3 const H)
{
	float3 const F0_dielectric = float3(0.04);
	float3 const F0 = mix(F0_dielectric, data.albedo, data.metallic);
	
	float3 const F  = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
	float const NDF = distributionGGX(N, H, data.roughness);
	float const G   = geometrySmith(N, V, L, data.roughness);
	
	float3 const numerator  = NDF * G * F;
	float const denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
	float3 const specular   = numerator / denominator;
	
	float3 const kS = F;
	float3 const kD = (1.0 - kS) * (1.0 - data.metallic);
	
	const float PI = 3.14159265359;
	  
	float const NdotL = max(dot(N, L), 0.0);
	return (kD * data.albedo / PI + specular) * radiance * NdotL;
}

fragment FragmentData mainPassEditorFS(MainPassData in [[ stage_in ]],
									   SceneRenderData device const& scene [[ buffer(0) ]])
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
		
		float const dist = length(lightPosition - worldPosition);
		float const attenuation = 1.0 / (dist * dist);
		
		float3 const radiance = light.color * light.intensity * attenuation;
		
		lightAcc += PBRLighting(data, radiance, N, V, L, H);
	}
	
	// SpotLights
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
		
		float const dist = length(lightPosition - worldPosition);
		float const attenuation = 1.0 / (dist * dist);
		
		float3 const radiance = intensity * light.color * light.intensity * attenuation;
		
		lightAcc += PBRLighting(data, radiance, N, V, L, H);
	}
	
	float3 const ambient    = 0.03 * data.albedo * data.ao;
	float3 const finalColor = ambient + lightAcc;
	
	result.color = float4(finalColor, 1);
	
	return result;
}


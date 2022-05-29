#include <metal_stdlib>
using namespace metal;

#include "PBRShader.h"

namespace bloom {

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

	static float geometrySmith(float3 N, float3 V, float3 L, float roughness) {
		float NdotV = max(dot(N, V), 0.0);
		float NdotL = max(dot(N, L), 0.0);
		float ggx2  = geometrySchlickGGX(NdotV, roughness);
		float ggx1  = geometrySchlickGGX(NdotL, roughness);
		
		return ggx1 * ggx2;
	}


	float3 PBRLighting(PBRData data,
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

}

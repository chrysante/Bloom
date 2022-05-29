#pragma once

#include "Bloom/Graphics/ShaderBase.hpp"
#include "Bloom/Graphics/Lights.hpp"

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
	
	struct DebugDrawData {
		float selectionLineWidth;
		bool visualizeShadowCascades;
		metal::float4x4 shadowCascadeVizTransforms[10];
		uint shadowCascadeVizCount;
	};
	
	BLOOM_SHADER_CONSTANT int maxShadowCascades = 10; // do we need this?
	
	BLOOM_SHADER_CONSTANT int maxShadowCasters = 32;
	
	struct alignas(metal::float4x4) ShadowRenderData {
		int numShadowCasters;
		int numCascades[maxShadowCasters];
	};
	
}

#pragma once

#include "ShaderBase.hpp"
#include "Lights.hpp"

namespace bloom {
	
	struct SceneRenderData {
		metal::float4x4 camera;
		metal::float3 cameraPosition;
		metal::float2 screenSize;
		metal::float2 screenResolution;
		
		uint numPointLights;
		RenderPointLight pointLights[32];
		
		uint numSpotLights;
		RenderSpotLight spotLights[32];
		
		uint numDirLights;
		RenderDirectionalLight dirLights[32];
	};
	
	struct DebugDrawData {
		float selectionLineWidth;
		bool visualizeShadowCascades;
		uint shadowCasterIndex;
		uint shadowMapOffset;
	};
	
	BLOOM_SHADER_CONSTANT int maxShadowCascades = 10;
	
	BLOOM_SHADER_CONSTANT int maxShadowCasters = 32;
	
	struct alignas(metal::float4x4) ShadowRenderData {
		int numShadowCasters;
		int numCascades[maxShadowCasters];
	};
	
}

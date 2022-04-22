#pragma once

#include "ShaderBase.hpp"
#include "Lights.hpp"

namespace bloom {
	
	struct SceneRenderData {
		metal::float4x4 camera;
		metal::float3 cameraPosition;
		metal::float2 screenSize;
		metal::float2 screenResolution;
		float selectionLineWidth;
		
		uint numPointLights;
		RenderPointLight pointLights[32];
		
		uint numSpotLights;
		RenderSpotLight spotLights[32];
	};
	
}

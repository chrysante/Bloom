#pragma once

#include "ShaderBase.hpp"

namespace bloom {
	
	enum struct LightType {
		pointlight, spotlight, directional
	};
	
	struct PointLight {
		metal::packed_float3 color;
		float intensity;
	};
	
	struct RenderPointLight {
		PointLight light;
		metal::float3 position;
	};
	
	struct SpotLight {
		metal::packed_float3 color;
		float intensity;
		float innerCutoff;
		float outerCutoff;
	};
	
	struct RenderSpotLight {
		SpotLight light;
		metal::float3 position;
		metal::float3 direction;
	};
	
	
}

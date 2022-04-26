#pragma once

#include "ShaderBase.hpp"

#ifdef BLOOM_CPP
#include <string_view>
#endif

namespace bloom {
	
	enum struct LightType {
		pointlight = 0,
		spotlight = 1,
		directional = 2,
		_count
	};
	
#ifdef BLOOM_CPP
	BLOOM_API std::string_view toString(LightType);
#endif
	
	struct LightCommon {
		metal::packed_float3 color;
		float intensity;
	};
	
	struct PointLight {
		LightCommon common;
		float radius;
	};
	
	struct RenderPointLight {
		PointLight light;
		metal::float3 position;
	};
	
	struct SpotLight {
		LightCommon common;
		float innerCutoff;
		float outerCutoff;
		float radius;
	};
	
	struct RenderSpotLight {
		SpotLight light;
		metal::float3 position;
		metal::float3 direction;
	};
	
	struct DirectionalLight {
		LightCommon common;
		
		bool castsShadows;
		float shadowDistance;
		int numCascades = 1;
		float cascadeDistributionExponent = 2;
		float cascadeTransitionFraction = 0.05;
		float shadowDistanceFadeoutFraction = 0.05;
//		metal::uint2 shadowMapResolution = 512;
	};
	
	struct RenderDirectionalLight {
		DirectionalLight light;
		metal::float3 direction;
	};
	
	
}

#pragma once

#include "Bloom/Graphics/ShaderBase.hpp"

namespace bloom {
	
	struct BloomParameters {
		metal::float3 curve;
		float contribution;
		float intensity;
		float scale;
		float threshold;
	};
	
	struct PostProcessParameters {
		BloomParameters bloom;
	};
	
	struct ForwardRendererParameters {
		PostProcessParameters postprocess;
	};
	
}

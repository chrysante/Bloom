#pragma once

#include "Bloom/Graphics/ShaderBase.hpp"


namespace bloom {
	
	struct Vertex3D {
		metal::packed_float3 position;
		metal::packed_float3 normal;
		metal::packed_float3 tangent;
		metal::packed_float3 binormal;
		metal::float4 color;
		metal::float2 textureCoordinates[4];
	};
	
}

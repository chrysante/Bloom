#include "Bloom/Graphics/ShaderBase.hpp"


namespace bloom {
	
	struct Vertex3D {
		packed_float3 position;
		packed_float3 normal;
		packed_float4 color;
		float2 textureCoordinates[8];
	};
	
}

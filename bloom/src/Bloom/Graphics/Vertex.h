#ifndef BLOOM_GRAPHICS_VERTEX_H
#define BLOOM_GRAPHICS_VERTEX_H

#include "Bloom/Graphics/ShaderBase.h"

namespace bloom {

struct Vertex3D {
    metal::packed_float3 position;
    metal::packed_float3 normal;
    metal::packed_float3 tangent;
    metal::packed_float3 binormal;
    metal::float4 color;
    metal::float2 textureCoordinates[4];
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_VERTEX_H

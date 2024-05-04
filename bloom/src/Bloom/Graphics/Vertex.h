#ifndef BLOOM_GRAPHICS_VERTEX_H
#define BLOOM_GRAPHICS_VERTEX_H

#include "Bloom/Shader/Common.h"

namespace bloom {

struct Vertex3D {
    vml::packed_float3 position;
    vml::packed_float3 normal;
    vml::packed_float3 tangent;
    vml::packed_float3 binormal;
    vml::float4 color;
    vml::float2 textureCoordinates[4];
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_VERTEX_H

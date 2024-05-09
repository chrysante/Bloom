#ifndef BLOOM_SHADER_VERTEX_H
#define BLOOM_SHADER_VERTEX_H

#include "Shader/Common.h"

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

#endif // BLOOM_SHADER_VERTEX_H

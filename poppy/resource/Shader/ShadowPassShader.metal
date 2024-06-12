#include <metal_stdlib>

#include "Shader/Vertex.h"

using namespace metal;

struct ShadowPassInOut {
    float4 position [[position]];
    uint layer [[render_target_array_index]];
};

vertex ShadowPassInOut rgShadowVertexShader(
    bloom::Vertex3D device const* vertices [[buffer(2)]],
    uint vertexID [[vertex_id]], uint shadowMapIndex [[instance_id]],
    float4x4 device const* lightSpaceTransforms [[buffer(5)]],
    float4x4 device const& objectTransform [[buffer(1)]])
{
    bloom::Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 positionLS = lightSpaceTransforms[shadowMapIndex] * objectTransform *
                        vertexPositionMS;
    return { .position = positionLS, .layer = shadowMapIndex };
}

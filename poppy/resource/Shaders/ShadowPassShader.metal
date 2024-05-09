#include "Bloom/Graphics/Vertex.h"
#include <metal_stdlib>

using namespace metal;

struct ShadowPassInOut {
    float4 position [[position]];
    uint layer [[render_target_array_index]];
};

vertex ShadowPassInOut rgShadowVertexShader(
    bloom::Vertex3D device const* vertices [[buffer(0)]],
    uint vertexID [[vertex_id]], uint shadowMapIndex [[instance_id]],
    float4x4 device const* lightSpaceTransforms [[buffer(1)]],
    float4x4 device const& objectTransform [[buffer(2)]])
{
    bloom::Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 positionLS = lightSpaceTransforms[shadowMapIndex] * objectTransform *
                        vertexPositionMS;
    return { .position = positionLS, .layer = shadowMapIndex };
}

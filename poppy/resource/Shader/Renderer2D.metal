#include <metal_stdlib>

#include "Shader/SharedTypes.h"
#include "Shader/Vertex.h"

using namespace metal;
using namespace bloom;

struct Vertex2DCS {
    float4 position [[position]];
    float4 color;
};

vertex Vertex2DCS Renderer2DWorldVS(CameraInput device const& cameraIn
                                    [[buffer(0)]],
                                    LineVertex2D device const* vertices
                                    [[buffer(1)]],
                                    uint vertexID [[vertex_id]])
{
    float aspect = cameraIn.viewportSize.x / cameraIn.viewportSize.y;
    auto v = vertices[vertexID];
    float4 posProj = cameraIn.camera * float4(v.position, 1);
    float4 nextPosProj = cameraIn.camera * float4(v.nextPosition, 1);
    float2 posScreen = float2(aspect, 1) * posProj.xy / posProj.z;
    float2 nextPosScreen = float2(aspect, 1) * nextPosProj.xy / nextPosProj.z;
    float2 dirScreen = normalize(nextPosScreen - posScreen);
    float2 N = { -dirScreen.y, dirScreen.x };
    float2 width = v.thickness / (2 * float2(aspect, 1));
    bool widthInScreenCoords = true;
    if (widthInScreenCoords) {
        // Screen coords
        // Transform width back into projected coordinates so we end up with the
        // same width regardless of camera position
        float2 widthProj = width * 2 * posProj.w / cameraIn.viewportSize.y;
        posProj.xy += N * widthProj * v.orientation;
    }
    else {
        // World coords
        posProj.xy += N * width * v.orientation;
    }
    return { posProj, float4(v.color) };
}

fragment float4 Renderer2DFS(Vertex2DCS in [[stage_in]]) { return in.color; }

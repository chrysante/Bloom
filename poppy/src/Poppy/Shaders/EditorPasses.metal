#include <metal_stdlib>

#include "Bloom/Graphics/MaterialProperties.h"
#include "Bloom/Graphics/Vertex.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace metal;
using namespace bloom;

vertex float4 rgSelectionPassVS(CameraInput device const& camera [[buffer(0)]],
                                Vertex3D device const* vertices [[buffer(1)]],
                                float4x4 device const& transform [[buffer(2)]],
                                uint vertexID [[vertex_id]])
{
    float4 vertexPositionMS = float4(vertices[vertexID].position, 1);
    float4 vertexPositionWS = transform * vertexPositionMS;
    return camera.camera * vertexPositionWS;
}

fragment float rgSelectionPassFS() { return 1; }

fragment uint32_t rgIDPassFS(uint32_t device const& ID [[buffer(0)]])
{
    return ID;
}

namespace {

struct EdgeDetectResult {
    float edge;
    bool occluded;
};

} // namespace

static EdgeDetectResult edgeDetecSampleOffset(
    texture2d<float> selectionMap, texture2d<float> depthMap,
    texture2d<float> selectionDepthMap, sampler sampler2D, float2 uv)
{
    float depth = depthMap.sample(sampler2D, uv).r;
    float selectionDepth = selectionDepthMap.sample(sampler2D, uv).r;
    return {
        .edge = selectionMap.sample(sampler2D, uv).r,
        .occluded = selectionDepth > depth,
    };
}

static EdgeDetectResult edgeDetect(texture2d<float> selectionMap,
                                   texture2d<float> depthMap,
                                   texture2d<float> selectionDepthMap,
                                   sampler sampler2D, float2 uv,
                                   float lineWidth, float2 screenRes)
{
    float2 uvOffset = lineWidth / screenRes;
    bool occluded = true;
    float edgeX = 0;
    EdgeDetectResult result;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(-uvOffset.x, -uvOffset.y));
    edgeX += result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D, uv + float2(-uvOffset.x, 0));
    edgeX += 2 * result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(-uvOffset.x, uvOffset.y));
    edgeX += result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(uvOffset.x, -uvOffset.y));
    edgeX -= result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D, uv + float2(uvOffset.x, 0));
    edgeX -= 2 * result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(uvOffset.x, uvOffset.y));
    edgeX -= result.edge;
    occluded &= result.occluded;
    float edgeY = 0;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(-uvOffset.x, -uvOffset.y));
    edgeY += result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D, uv + float2(0, -uvOffset.y));
    edgeY += 2 * result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(uvOffset.x, -uvOffset.y));
    edgeY += result.edge;
    occluded &= result.occluded;

    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(-uvOffset.x, uvOffset.y));
    edgeY -= result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D, uv + float2(0, uvOffset.y));
    edgeY -= 2 * result.edge;
    occluded &= result.occluded;
    result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
                                   sampler2D,
                                   uv + float2(uvOffset.x, uvOffset.y));
    edgeY -= result.edge;
    occluded &= result.occluded;
    float edge = min(abs(edgeX) + abs(edgeY), 1.0);
    return { edge, occluded };
}

kernel void rgEditorComposition(
    texture2d<float, access::write> dest [[texture(0)]],
    texture2d<float, access::read> sceneColor [[texture(1)]],
    texture2d<float, access::sample> sceneDepth [[texture(2)]],
    texture2d<float, access::sample> selectionMap [[texture(3)]],
    texture2d<float, access::sample> selectionDepth [[texture(4)]],
    uint2 position [[thread_position_in_grid]])
{
    float3 color = sceneColor.read(position).rgb;
    constexpr sampler texSampler(mag_filter::linear, min_filter::linear);
    float2 screenSize = { (float)dest.get_width(), (float)dest.get_height() };
    float2 UV = (float2)position / screenSize;
    auto edgeResult = edgeDetect(selectionMap, sceneDepth, selectionDepth,
                                 texSampler, UV,
                                 /* selectionLineWidth: */ 4, screenSize);
    float edge = edgeResult.occluded ? edgeResult.edge / 3 : edgeResult.edge;
    float3 selectionLineColor = { 1, 0.5, 0 };
    float3 result = mix(color, selectionLineColor, edge);
    dest.write(float4(result, 1), position);
}

struct GridVertex {
    float4 position [[position]];
    float2 UV;
};

static constant float2 GridUVs[] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };

static constant float GridSize = 2000;
static constant float GridLineWidth = 2.5;
static constant float NumLines = 20;

vertex GridVertex rgGridPassVS(CameraInput device const& camera [[buffer(0)]],
                               uint const vertexID [[vertex_id]])
{
    float4 vertexPositionWS =
        float4(GridSize * (GridUVs[vertexID] - 0.5f), 0, 1);
    return { .position = camera.camera * vertexPositionWS,
             .UV = GridUVs[vertexID] };
}

static float gridLines(float UV)
{
    float uvLineWidth = GridLineWidth / (GridSize / NumLines);
    UV *= 1 + (GridLineWidth / GridSize);
    UV -= (GridLineWidth / GridSize) / 2;
    UV *= NumLines;
    float x = fract(UV);
    return max(1 - step(uvLineWidth / 2, x), step(1 - uvLineWidth / 2, x));
}

fragment float4 rgGridPassFS(GridVertex in [[stage_in]])
{
    return mix(float4(), float4(0.5, 0.5, 0.5, 1.0),
               max(gridLines(in.UV.x), gridLines(in.UV.y)));
}

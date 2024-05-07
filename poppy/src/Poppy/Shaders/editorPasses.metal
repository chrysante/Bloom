#include <metal_stdlib>

#include "Bloom/Graphics/MaterialProperties.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/Vertex.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Shaders/Lights.h"
#include "Poppy/Shaders/PostProcess.h"

using namespace metal;
using namespace bloom;

vertex float4 selectionPassVS(SceneRenderParameters device const& scene
                              [[buffer(0)]],
                              Vertex3D device const* vertices [[buffer(1)]],
                              float4x4 device const& transform [[buffer(2)]],
                              uint const vertexID [[vertex_id]])
{
    Vertex3D v = vertices[vertexID];
    float4 vertexPositionMS = float4(v.position, 1);
    float4 vertexPositionWS = transform * vertexPositionMS;
    return scene.camera * vertexPositionWS;
}

fragment float selectionPassFS() { return 1; }

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
    EdgeDetectResult result;
    result.edge = selectionMap.sample(sampler2D, uv).r;
    float depth = depthMap.sample(sampler2D, uv).r;
    float selectionDepth = selectionDepthMap.sample(sampler2D, uv).r;
    result.occluded = selectionDepth > depth;
    return result;
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

struct EditorPassData {
    float4 positionCS [[position]];
    float3 normalWS;
    float3 positionWS;
    float3 color;
};

vertex EditorPassData editorPassVS(SceneRenderParameters device const& scene
                                   [[buffer(0)]],
                                   Vertex3D device const* vertices
                                   [[buffer(1)]],
                                   float4x4 device const& transform
                                   [[buffer(2)]],
                                   uint const vertexID [[vertex_id]])
{
    Vertex3D const v = vertices[vertexID];
    float4 const vertexPositionMS = float4(v.position, 1);
    float4 const vertexNormalMS = float4(v.normal, 0);
    float4 const vertexPositionWS = transform * vertexPositionMS;
    EditorPassData result;
    result.positionCS = scene.camera * vertexPositionWS;
    result.normalWS = (transform * vertexNormalMS).xyz;
    result.positionWS = vertexPositionWS.xyz;
    result.color = v.color.rgb;
    return result;
}

struct EditorPassFragmentData {
    float4 shadowCascade [[color(0)]];
};

fragment EditorPassFragmentData
    editorPassFS(EditorPassData in [[stage_in]],
                 SceneRenderParameters device const& scene [[buffer(0)]],
                 DebugDrawData device const& debugData [[buffer(1)]],
                 ShadowRenderData device const& shadowData [[buffer(2)]],
                 float4x4 device const* lightSpaceTransforms [[buffer(3)]],
                 texture2d_array<float> shadowMaps [[texture(0)]],
                 sampler shadowMapSampler [[sampler(0)]])
{
    EditorPassFragmentData result;
    // Editor Properties
    if (debugData.visualizeShadowCascades) {
        float3 shadowCascade =
            visualizeShadowCascade(0, debugData.shadowCascadeVizCount,
                                   debugData.shadowCascadeVizTransforms,
                                   float4(in.positionWS, 1));
        result.shadowCascade = float4(shadowCascade, 1);
    }
    return result;
}

fragment float4 wireframePassFS() { return float4(1, 1, 1, 1); }

struct GridVertex {
    float4 position [[position]];
    float2 UV;
};

vertex GridVertex rgGridPassVS(CameraInput device const& camera [[buffer(0)]],
                               float2 device const* UVs [[buffer(1)]],
                               uint const vertexID [[vertex_id]])
{
    float size = 1000;
    float2 pos = 2.0f * UVs[vertexID] - 1.0f;
    float4 vertexPositionWS = float4(size * pos, 0, 1);
    return { .position = camera.camera * vertexPositionWS,
             .UV = UVs[vertexID] };
}

fragment float4 gridPassFS(GridVertex in [[stage_in]])
{
    return float4(in.UV, 0, 1);
}

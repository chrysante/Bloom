#include <metal_stdlib>

#include "Bloom/Graphics/MaterialProperties.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/Vertex.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Renderer/EditorDrawData.h"
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

fragment float4 editorCompositionFS(
    PPRasterOutput in [[stage_in]],
    bloom::SceneRenderParameters device const& scene [[buffer(0)]],
    poppy::EditorDrawData device const& editor [[buffer(1)]],
    texture2d<float> postProcessed [[texture(0)]],
    texture2d<float> depthMap [[texture(1)]],
    texture2d<float> selectionMap [[texture(2)]],
    texture2d<float> selectionDepthMap [[texture(3)]],
    sampler sampler2D [[sampler(0)]])
{
    float3 result = postProcessed.sample(sampler2D, in.screenCoords).rgb;
    float depth = depthMap.sample(sampler2D, in.screenCoords).r;
    (void)depth;
    if (editor.overlayDrawDesc.drawSelection) {
        auto edgeResult = edgeDetect(selectionMap, depthMap, selectionDepthMap,
                                     sampler2D, in.screenCoords,
                                     editor.overlayDrawDesc.selectionLineWidth,
                                     scene.screenResolution);
        float edge = edgeResult.occluded ? edgeResult.edge / 3 :
                                           edgeResult.edge;
        result =
            mix(result, editor.overlayDrawDesc.selectionLineColor.rgb, edge);
    }
    return float4(result, 1);
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

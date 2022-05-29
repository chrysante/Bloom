#include <metal_stdlib>

#include "PostProcess.h"

#include "Bloom/Graphics/Renderer/SceneRenderData.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "Bloom/Graphics/Vertex.hpp"

#include "Poppy/Renderer/EditorDrawData.hpp"

using namespace metal;
using namespace bloom;

vertex float4 selectionPassVS(SceneRenderData device const&  scene     [[ buffer(0) ]],
									  Vertex3D device const* vertices  [[ buffer(1) ]],
									  float4x4 device const& transform [[ buffer(2) ]],

									  uint const             vertexID  [[ vertex_id ]])
{
	Vertex3D const v = vertices[vertexID];
	
	float4 const vertexPositionMS = float4(v.position, 1);
	float4 const vertexPositionWS = transform * vertexPositionMS;
	
	return scene.camera * vertexPositionWS;
}

fragment float selectionPassFS() {
	return 1;
}

namespace {
	struct EdgeDetectResult {
		float edge;
		bool occluded;
	};
}

static EdgeDetectResult edgeDetecSampleOffset(texture2d<float> selectionMap,
											  texture2d<float> depthMap,
											  texture2d<float> selectionDepthMap,
											  sampler sampler2D,
											  float2 uv)
{
	EdgeDetectResult result;
	result.edge = selectionMap.sample(sampler2D, uv).r;
	float const depth = depthMap.sample(sampler2D, uv).r;
	float const selectionDepth = selectionDepthMap.sample(sampler2D, uv).r;
	result.occluded = selectionDepth > depth;
	return result;
}

//static float edgeDetect(texture2d<float> selectionMap,
//						texture2d<float> depthMap,
//						texture2d<float> selectionDepthMap,
//						sampler sampler2D,
//						float2 uv,
//						float lineWidth,
//						float2 screenRes)
//{
//	float2 const uvOffset = lineWidth / screenRes;
//	bool occluded = true;
//	float edgeX = 0;
//	edgeX +=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x, -uvOffset.y)).r;
//	edgeX += 2 * selectionMap.sample(sampler2D, uv + float2(-uvOffset.x,  0)).r;
//	edgeX +=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x,  uvOffset.y)).r;
//
//	edgeX -=     selectionMap.sample(sampler2D, uv + float2(uvOffset.x, -uvOffset.y)).r;
//	edgeX -= 2 * selectionMap.sample(sampler2D, uv + float2(uvOffset.x,  0)).r;
//	edgeX -=     selectionMap.sample(sampler2D, uv + float2(uvOffset.x,  uvOffset.y)).r;
//
//	float edgeY = 0;
//	edgeY +=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x, -uvOffset.y)).r;
//	edgeY += 2 * selectionMap.sample(sampler2D, uv + float2(0,           -uvOffset.y)).r;
//	edgeY +=     selectionMap.sample(sampler2D, uv + float2( uvOffset.x, -uvOffset.y)).r;
//
//	edgeY -=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x,  uvOffset.y)).r;
//	edgeY -= 2 * selectionMap.sample(sampler2D, uv + float2(0,            uvOffset.y)).r;
//	edgeY -=     selectionMap.sample(sampler2D, uv + float2( uvOffset.x,  uvOffset.y)).r;
//
//	return min(abs(edgeX) + abs(edgeY), 1.0);
//}

static EdgeDetectResult edgeDetect(texture2d<float> selectionMap,
								   texture2d<float> depthMap,
								   texture2d<float> selectionDepthMap,
								   sampler sampler2D,
								   float2 uv,
								   float lineWidth,
								   float2 screenRes)
{
	float2 const uvOffset = lineWidth / screenRes;
	bool occluded = true;
	float edgeX = 0;
	EdgeDetectResult result;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(-uvOffset.x, -uvOffset.y));
	edgeX +=     result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(-uvOffset.x,  0));
	edgeX += 2 * result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(-uvOffset.x,  uvOffset.y));
	edgeX +=     result.edge;
	occluded &= result.occluded;

	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(uvOffset.x, -uvOffset.y));
	edgeX -=     result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(uvOffset.x,  0));
	edgeX -= 2 * result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(uvOffset.x,  uvOffset.y));
	edgeX -=     result.edge;
	occluded &= result.occluded;
	
	float edgeY = 0;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(-uvOffset.x, -uvOffset.y));
	edgeY +=     result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(0,           -uvOffset.y));
	edgeY += 2 * result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2( uvOffset.x, -uvOffset.y));
	edgeY +=     result.edge;
	occluded &= result.occluded;

	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(-uvOffset.x,  uvOffset.y));
	edgeY -=     result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2(0,            uvOffset.y));
	edgeY -= 2 * result.edge;
	occluded &= result.occluded;
	result = edgeDetecSampleOffset(selectionMap, depthMap, selectionDepthMap,
								   sampler2D, uv + float2( uvOffset.x,  uvOffset.y));
	edgeY -=     result.edge;
	occluded &= result.occluded;

	
	float const edge = min(abs(edgeX) + abs(edgeY), 1.0);
	
	return {
		edge, occluded
	};
}


fragment float4 editorCompositionFS(PPRasterOutput in                          [[ stage_in   ]],
									bloom::SceneRenderData device const& scene [[ buffer(0)  ]],
									poppy::EditorDrawData device const& editor [[ buffer(1)  ]],
									texture2d<float> postProcessed             [[ texture(0) ]],
									texture2d<float> depthMap                  [[ texture(1) ]],
									texture2d<float> selectionMap              [[ texture(2) ]],
									texture2d<float> selectionDepthMap         [[ texture(3) ]],
									sampler          sampler2D                 [[ sampler(0) ]])
{
	float3 result = postProcessed.sample(sampler2D, in.screenCoords).rgb;
//	float const depth = depthMap.sample(sampler2D, in.screenCoords).r;
	if (editor.overlayDrawDesc.drawSelection) {
		auto const edgeResult = edgeDetect(selectionMap,
										   depthMap,
										   selectionDepthMap,
										   sampler2D,
										   in.screenCoords,
										   editor.overlayDrawDesc.selectionLineWidth,
										   scene.screenResolution);
		float const edge = edgeResult.occluded ? edgeResult.edge / 3 : edgeResult.edge;
		
		result = mix(result, editor.overlayDrawDesc.selectionLineColor.rgb, edge);
	}
	
	return float4(result, 1);
}




//struct EditorPassData {
//	float4 positionCS [[ position ]];
//
//	float3 normalWS;
//	float3 positionWS;
//	float3 color;
//};
//
//vertex EditorPassData editorPassVS(SceneRenderData device const&  scene     [[ buffer(0) ]],
//
//								   Vertex3D device const*         vertices  [[ buffer(1) ]],
//								   float4x4 device const&         transform [[ buffer(2) ]],
//
//								   uint const                     vertexID  [[ vertex_id ]])
//{
//	Vertex3D const v = vertices[vertexID];
//
//	float4 const vertexPositionMS = float4(v.position, 1);
//	float4 const vertexNormalMS   = float4(v.normal, 0);
//	float4 const vertexPositionWS = transform * vertexPositionMS;
//
//	EditorPassData result;
//
//	result.positionCS = scene.camera * vertexPositionWS;
//	result.normalWS   = (transform * vertexNormalMS).xyz;
//	result.positionWS = vertexPositionWS.xyz;
//	result.color      = v.color.rgb;
//
//	return result;
//}
//
//struct EditorPassFragmentData {
//	float4 shadowCascade [[ color(0) ]];
//};
//
//fragment EditorPassFragmentData editorPassFS(EditorPassData in                           [[ stage_in   ]],
//											 SceneRenderData device const& scene         [[ buffer(0)  ]],
//											 DebugDrawData device const& debugData       [[ buffer(1)  ]],
//											 ShadowRenderData device const& shadowData   [[ buffer(2)  ]],
//											 float4x4 device const* lightSpaceTransforms [[ buffer(3)  ]],
//											 texture2d_array<float> shadowMaps           [[ texture(0) ]],
//											 sampler shadowMapSampler                    [[ sampler(0) ]])
//{
//	EditorPassFragmentData result;
//
//	// Editor Properties
//	if (debugData.visualizeShadowCascades) {
//		float3 const shadowCascade = visualizeShadowCascade(0,
//															debugData.shadowCascadeVizCount,
//															debugData.shadowCascadeVizTransforms,
//															float4(in.positionWS, 1));
//		result.shadowCascade = float4(shadowCascade, 1);
//	}
//
//	return result;
//}
//
//fragment float4 wireframePassFS() {
//	return float4(1, 1, 1, 1);
//}

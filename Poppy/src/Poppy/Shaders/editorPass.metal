#include <metal_stdlib>
#include "Bloom/Graphics/SceneRenderData.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "Bloom/Graphics/Vertex.hpp"
#include "Bloom/Graphics/EntityRenderData.hpp"

#include "PBRShader.h"
#include "Lights.h"

using namespace metal;
using namespace bloom;

struct EditorPassData {
	float4 positionCS [[ position ]];
	
	float3 normalWS;
	float3 positionWS;
	float3 color;
	
	uint entityID;
};

vertex EditorPassData editorPassVS(SceneRenderData device const&  scene    [[ buffer(0) ]],
									 
								   Vertex3D device const*         vertices [[ buffer(1) ]],
								   EntityRenderData device const& entity   [[ buffer(2) ]],
								   
								   uint const                     vertexID [[ vertex_id ]])
{
	Vertex3D const v = vertices[vertexID];
	
	float4 const vertexPositionMS = float4(v.position, 1);
	float4 const vertexNormalMS   = float4(v.normal, 0);
	float4 const vertexPositionWS = entity.transform * vertexPositionMS;
	
	EditorPassData result;
	
	result.positionCS = scene.camera * vertexPositionWS;
	result.normalWS   = (entity.transform * vertexNormalMS).xyz;
	result.positionWS = vertexPositionWS.xyz;
	result.color      = v.color.rgb;
	result.entityID   = entity.ID;
	
	return result;
}

struct EditorPassFragmentData {
	uint entityID [[ color(0) ]];
	float4 shadowCascade [[ color(1) ]];
};

fragment EditorPassFragmentData editorPassFS(EditorPassData in                           [[ stage_in   ]],
											 SceneRenderData device const& scene         [[ buffer(0)  ]],
											 DebugDrawData device const& debugData       [[ buffer(1)  ]],
											 ShadowRenderData device const& shadowData   [[ buffer(2)  ]],
											 float4x4 device const* lightSpaceTransforms [[ buffer(3)  ]],
											 texture2d_array<float> shadowMaps           [[ texture(0) ]],
											 sampler shadowMapSampler                    [[ sampler(0) ]])
{
	EditorPassFragmentData result;
	
	// Editor Properties
	result.entityID = in.entityID;
	if (debugData.visualizeShadowCascades) {
		float3 const shadowCascade = visualizeShadowCascade(0,
															debugData.shadowCascadeVizCount,
															debugData.shadowCascadeVizTransforms,
															float4(in.positionWS, 1));
		result.shadowCascade = float4(shadowCascade, 1);
	}
	
	return result;
}

fragment float4 wireframePassFS() {
	return float4(1, 1, 1, 1);
}

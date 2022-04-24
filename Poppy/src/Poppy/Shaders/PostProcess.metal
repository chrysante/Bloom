#include <metal_stdlib>

#include "Bloom/Graphics/SceneRenderData.hpp"

using namespace metal;

struct PPRasterOutput {
	float4 position [[ position ]];
	float2 screenCoords;
};

vertex PPRasterOutput postprocessVS(float2 device const* vertices [[ buffer(1) ]],
									uint const vertexID [[ vertex_id ]])
{
	float2 const v = vertices[vertexID];
	
	PPRasterOutput result;
	
	result.position = float4(v, 0, 1);
	result.screenCoords = (v + 1) / 2;
	result.screenCoords.y = 1 - result.screenCoords.y;
	
	return result;
}


fragment float4 postprocess(PPRasterOutput in                          [[ stage_in   ]],
							bloom::SceneRenderData device const& scene [[ buffer(0)  ]],
							texture2d<float> colorIn                   [[ texture(0) ]],
							sampler          sampler2D                 [[ sampler(0) ]])
{
	
	float3 const HDR = colorIn.sample(sampler2D, in.screenCoords).rgb;
	
	// reinhard tone mapping
	float3 const toneMapped = HDR / (HDR + 1.0);
	// gamma correction
	float3 const corrected = pow(toneMapped, 1.0 / 2.2);
	
	return float4(corrected, 1);
}

static float edgeDetect(texture2d<float> selectionMap, sampler sampler2D, float2 uv, float lineWidth, float2 screenRes) {
	float2 const uvOffset = lineWidth / screenRes;
	
	float edgeX = 0;
	edgeX +=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x, -uvOffset.y)).r;
	edgeX += 2 * selectionMap.sample(sampler2D, uv + float2(-uvOffset.x,  0)).r;
	edgeX +=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x,  uvOffset.y)).r;

	edgeX -=     selectionMap.sample(sampler2D, uv + float2(uvOffset.x, -uvOffset.y)).r;
	edgeX -= 2 * selectionMap.sample(sampler2D, uv + float2(uvOffset.x,  0)).r;
	edgeX -=     selectionMap.sample(sampler2D, uv + float2(uvOffset.x,  uvOffset.y)).r;

	float edgeY = 0;
	edgeY +=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x, -uvOffset.y)).r;
	edgeY += 2 * selectionMap.sample(sampler2D, uv + float2(0,           -uvOffset.y)).r;
	edgeY +=     selectionMap.sample(sampler2D, uv + float2( uvOffset.x, -uvOffset.y)).r;

	edgeY -=     selectionMap.sample(sampler2D, uv + float2(-uvOffset.x,  uvOffset.y)).r;
	edgeY -= 2 * selectionMap.sample(sampler2D, uv + float2(0,            uvOffset.y)).r;
	edgeY -=     selectionMap.sample(sampler2D, uv + float2( uvOffset.x,  uvOffset.y)).r;
	
	return min(abs(edgeX) + abs(edgeY), 1.0);
}

fragment float4 editorPP(PPRasterOutput in                          [[ stage_in   ]],
						 bloom::SceneRenderData device const& scene [[ buffer(0)  ]],
						 texture2d<float> finalImage                [[ texture(0) ]],
						 texture2d<float> selectionMap              [[ texture(1) ]],
						 sampler          sampler2D                 [[ sampler(0) ]])
{
	float const edge = edgeDetect(selectionMap,
								  sampler2D,
								  in.screenCoords,
								  scene.selectionLineWidth,
								  scene.screenResolution);
	
	float3 const imageColor = finalImage.sample(sampler2D, in.screenCoords).rgb;
	
	float3 const finalColor = mix(imageColor, float3(1, 0.5, 0), edge);
	
	return float4(finalColor, 1);
}

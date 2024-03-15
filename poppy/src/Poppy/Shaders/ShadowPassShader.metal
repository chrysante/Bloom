#include <metal_stdlib>
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/Vertex.h"

using namespace metal;


struct ShadowPassInOut {
	float4 position [[ position ]];
	uint layer [[ render_target_array_index ]];
};

vertex ShadowPassInOut shadowVertexShader(bloom::Vertex3D device const* vertices           [[ buffer(1) ]],
										  uint const vertexID                              [[ vertex_id ]],
										  uint const shadowMapIndex                        [[ instance_id ]],
										  bloom::SceneRenderData device const& scene       [[ buffer(0) ]],
										  float4x4 device const* lightSpaceTransforms      [[ buffer(3) ]],
										  float4x4 device const& objectTransform           [[ buffer(2) ]])
{
	bloom::Vertex3D const v = vertices[vertexID];
	float4 const vertexPositionMS = float4(v.position, 1);
	float4 const positionLS = lightSpaceTransforms[shadowMapIndex] * objectTransform * vertexPositionMS;
	
	ShadowPassInOut result;
	result.position = positionLS;
	result.layer = shadowMapIndex;
	
	return result;
}

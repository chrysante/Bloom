#include <metal_stdlib>
#include "Bloom/Graphics/Renderer/SceneRenderData.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "Bloom/Graphics/Vertex.hpp"

using namespace metal;

vertex float4 outlinePassVS(bloom::Vertex3D device const* vertices [[ buffer(1) ]],
							uint const vertexID [[ vertex_id ]],
							bloom::SceneRenderData device const& scene [[ buffer(0) ]],
							float4x4 device const& transform [[ buffer(2) ]])
{
	bloom::Vertex3D const v = vertices[vertexID];
	float4 const vertexPositionMS = float4(v.position, 1);
	return scene.camera * transform * vertexPositionMS;
}

fragment float outlinePassFS(float4 in [[ stage_in ]]) {
	return 1.0;
}



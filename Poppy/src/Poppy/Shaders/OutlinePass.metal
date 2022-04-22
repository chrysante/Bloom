#include <metal_stdlib>
#include "Bloom/Graphics/SceneRenderData.hpp"
#include "Bloom/Graphics/MaterialProperties.hpp"
#include "Bloom/Graphics/Vertex.hpp"
#include "Bloom/Graphics/EntityRenderData.hpp"

using namespace metal;

vertex float4 outlinePassVS(bloom::Vertex3D device const* vertices [[ buffer(1) ]],
							uint const vertexID [[ vertex_id ]],
							bloom::SceneRenderData device const& scene [[ buffer(0) ]],
							bloom::EntityRenderData device const& entity [[ buffer(2) ]])
{
	bloom::Vertex3D const v = vertices[vertexID];
	float4 const vertexPositionMS = float4(v.position, 1);
	return scene.camera * entity.transform * vertexPositionMS;
}

fragment float outlinePassFS(float4 in [[ stage_in ]]) {
	return 1.0f;
}



#include "ExampleScene.hpp"

#include <mtl/mtl.hpp>
using namespace mtl::short_types;

#include "Bloom/Scene/Components.hpp"
#include "Bloom/Graphics/Vertex.hpp"

#include <utl/stdio.hpp>

namespace poppy {
	
	static auto makeMaterial(bloom::RenderContext& renderContext) {
		return bloom::Material::makeDefaultMaterial(&renderContext);
	}
	
	/// MARK: - Cube
	static auto makeCubeMesh(bloom::RenderContext& renderContext) {
		bloom::Vertex3D vertices[] = {
			{ .position = { -50, -50, -50 }, .normal = {  0,  0, -1 }, .color = { 0, 0, 0, 1 } }, // bottom
			{ .position = {  50, -50, -50 }, .normal = {  0,  0, -1 }, .color = { 1, 0, 0, 1 } },
			{ .position = { -50,  50, -50 }, .normal = {  0,  0, -1 }, .color = { 0, 1, 0, 1 } },
			{ .position = {  50,  50, -50 }, .normal = {  0,  0, -1 }, .color = { 1, 1, 0, 1 } },
			{ .position = { -50, -50,  50 }, .normal = {  0,  0,  1 }, .color = { 0, 0, 1, 1 } }, // top
			{ .position = {  50, -50,  50 }, .normal = {  0,  0,  1 }, .color = { 1, 0, 1, 1 } },
			{ .position = { -50,  50,  50 }, .normal = {  0,  0,  1 }, .color = { 0, 1, 1, 1 } },
			{ .position = {  50,  50,  50 }, .normal = {  0,  0,  1 }, .color = { 1, 1, 1, 1 } },
			{ .position = { -50, -50, -50 }, .normal = { -1,  0,  0 }, .color = { 0, 0, 0, 1 } }, // left
			{ .position = { -50,  50, -50 }, .normal = { -1,  0,  0 }, .color = { 0, 1, 0, 1 } },
			{ .position = { -50, -50,  50 }, .normal = { -1,  0,  0 }, .color = { 0, 0, 1, 1 } },
			{ .position = { -50,  50,  50 }, .normal = { -1,  0,  0 }, .color = { 0, 1, 1, 1 } },
			{ .position = {  50, -50, -50 }, .normal = {  1,  0,  0 }, .color = { 1, 0, 0, 1 } }, // right
			{ .position = {  50,  50, -50 }, .normal = {  1,  0,  0 }, .color = { 1, 1, 0, 1 } },
			{ .position = {  50, -50,  50 }, .normal = {  1,  0,  0 }, .color = { 1, 0, 1, 1 } },
			{ .position = {  50,  50,  50 }, .normal = {  1,  0,  0 }, .color = { 1, 1, 1, 1 } },
			{ .position = { -50, -50, -50 }, .normal = {  0, -1,  0 }, .color = { 0, 0, 0, 1 } }, // front
			{ .position = {  50, -50, -50 }, .normal = {  0, -1,  0 }, .color = { 1, 0, 0, 1 } },
			{ .position = { -50, -50,  50 }, .normal = {  0, -1,  0 }, .color = { 0, 0, 1, 1 } },
			{ .position = {  50, -50,  50 }, .normal = {  0, -1,  0 }, .color = { 1, 0, 1, 1 } },
			{ .position = { -50,  50, -50 }, .normal = {  0,  1,  0 }, .color = { 0, 1, 0, 1 } }, // back
			{ .position = {  50,  50, -50 }, .normal = {  0,  1,  0 }, .color = { 1, 1, 0, 1 } },
			{ .position = { -50,  50,  50 }, .normal = {  0,  1,  0 }, .color = { 0, 1, 1, 1 } },
			{ .position = {  50,  50,  50 }, .normal = {  0,  1,  0 }, .color = { 1, 1, 1, 1 } },
		};
		
		std::uint32_t indices[] = {
			0, 1, 2, 1, 3, 2, // bottom
			4, 6, 5, 5, 6, 7, // top
			8, 9, 10, 9, 11, 10, // left
			13, 12, 14, 14, 15, 13, // right
			16, 18, 19, 16, 19, 17,
			20, 21, 23, 20, 23, 22
		};
		
		auto mesh = bloom::StaticRenderMesh::create();
		mesh->vertexBuffer = renderContext.createVertexBuffer(vertices, sizeof vertices);
		mesh->indexBuffer = renderContext.createIndexBuffer(indices);
		return mesh;
	}
	
	/// MARK: - Sphere
	static std::size_t indexOfVertexAt(std::size_t i, std::size_t j,
									   usize2 const& vertexResolution)
	{
		return j * vertexResolution.x + i;
	}
	
	template <int A, int B>
	static void calculatePlaneVertices(bloom::Vertex3D* vertexData, mtl::usize2 vertexResolution, float3 position)
	{
		constexpr int C = 3 - A - B;
		float const scale = 10 / (vertexResolution.x - 1);
		
		for (std::size_t j = 0; j < vertexResolution.y; ++j) {
			for (std::size_t i = 0; i < vertexResolution.x; ++i) {
				auto& vertex = vertexData[indexOfVertexAt(i, j, vertexResolution)];
				vertex.position[A] = i * scale + position[A];
				vertex.position[B] = j * scale + position[B];
				vertex.position[C] = position[C];
				vertex.color = 1;
			}
		}
	}
	
	static void calculatePlaneIndices(std::uint32_t* indexData, mtl::usize2 quadResolution,
									  std::size_t offset, bool frontFacing) {
		usize2 const vertexResolution = quadResolution + 1;
		
		std::size_t k = 0;
		for (std::size_t j = 0; j < quadResolution.y; ++j) {
			for (std::size_t i = 0; i < quadResolution.x; ++i) {
				// loop over all quads in row major order
				if (frontFacing) {
					indexData[k + 0] = offset + indexOfVertexAt(i,     j,     vertexResolution);
					indexData[k + 1] = offset + indexOfVertexAt(i + 1, j,     vertexResolution);
					indexData[k + 2] = offset + indexOfVertexAt(i,     j + 1, vertexResolution);
					indexData[k + 3] = offset + indexOfVertexAt(i,     j + 1, vertexResolution);
					indexData[k + 4] = offset + indexOfVertexAt(i + 1, j,     vertexResolution);
					indexData[k + 5] = offset + indexOfVertexAt(i + 1, j + 1, vertexResolution);
				}
				else {
					indexData[k + 0] = offset + indexOfVertexAt(i,     j,     vertexResolution);
					indexData[k + 2] = offset + indexOfVertexAt(i + 1, j,     vertexResolution);
					indexData[k + 1] = offset + indexOfVertexAt(i,     j + 1, vertexResolution);
					indexData[k + 3] = offset + indexOfVertexAt(i,     j + 1, vertexResolution);
					indexData[k + 5] = offset + indexOfVertexAt(i + 1, j,     vertexResolution);
					indexData[k + 4] = offset + indexOfVertexAt(i + 1, j + 1, vertexResolution);
				}
				
				k += 6;
			}
		}
	}
	
	static auto makeSphereMesh(bloom::RenderContext& renderContext, int const segments, float radius) {
		using namespace bloom;
		assert(segments > 0);
		mtl::usize2 const quadResolution = { segments, segments };
		std::size_t const faceIndexCount = quadResolution.x * quadResolution.y * 6;
		std::size_t const indexCount = faceIndexCount * 6;
		
		mtl::usize2 const vertexResolution = { segments + 1, segments + 1 };
		std::size_t const faceVertexCount = vertexResolution.x * vertexResolution.y;
		std::size_t const vertexCount = faceVertexCount * 6;
		
		utl::vector<Vertex3D> vertexData(vertexCount);
		utl::vector<std::uint32_t> indexData(indexCount);
		
		
		calculatePlaneVertices<0, 1>(vertexData.data() + 0 * faceVertexCount, vertexResolution, { -5, -5, -5 });
		calculatePlaneVertices<0, 1>(vertexData.data() + 1 * faceVertexCount, vertexResolution, { -5, -5,  5 });
		calculatePlaneVertices<1, 2>(vertexData.data() + 2 * faceVertexCount, vertexResolution, { -5, -5, -5 });
		calculatePlaneVertices<1, 2>(vertexData.data() + 3 * faceVertexCount, vertexResolution, {  5, -5, -5 });
		calculatePlaneVertices<2, 0>(vertexData.data() + 4 * faceVertexCount, vertexResolution, { -5, -5, -5 });
		calculatePlaneVertices<2, 0>(vertexData.data() + 5 * faceVertexCount, vertexResolution, { -5,  5, -5 });
		
		for (auto& vertex: vertexData) {
			auto const normal = mtl::fast_normalize(vertex.position);
			vertex.normal = normal;
			vertex.position = radius * normal;
		}
		
		for (int i = 0; i < 6; ++i) {
			calculatePlaneIndices(indexData.data() + i * faceIndexCount, quadResolution, i * faceVertexCount, i % 2 == 0);
		}
		
		
		auto result = bloom::StaticRenderMesh::create();
		result->vertexBuffer = renderContext.createVertexBuffer(vertexData.data(), sizeof(Vertex3D) * vertexCount);
		result->indexBuffer = renderContext.createIndexBuffer(indexData);
		return result;
	}
	
	void buildExampleScene(bloom::Scene& scene, bloom::RenderContext& renderContext) {
		using namespace bloom;
		auto cube = makeCubeMesh(renderContext);
		auto sphere = makeSphereMesh(renderContext, 10, 50);
		auto material = makeMaterial(renderContext);
		bloom::MeshRenderComponent cubeRenderer{ .material = material, .mesh = cube };
		bloom::MeshRenderComponent sphereRenderer{ .material = material, .mesh = sphere };
		
		// Lights
		{
			auto entity = scene.createEntity("Point Light");
			auto& transform = scene.getComponent<bloom::TransformComponent>(entity);
			transform.position = { 30, 10, 80 };
			auto light = bloom::LightComponent{ bloom::PointLight{
				{
					.color     = { 1, 0, 1 },
					.intensity = 5000
				},
				.radius = 5,
			}};
			scene.addComponent(entity, light);
		}
		
		{
			auto entity = scene.createEntity("Directional Light");
			auto& transform = scene.getComponent<bloom::TransformComponent>(entity);
			transform.orientation = { 1, 0, 0, 0 };
			auto light = bloom::LightComponent{ bloom::DirectionalLight{
				{
					.color     = { 1, 1, 1 },
					.intensity = 1
				},
				.castsShadows = true,
				.shadowDistance = 500
			}};
			scene.addComponent(entity, light);
		}
		
		{
			auto entity = scene.createEntity("Spot Light");
			auto& transform = scene.getComponent<bloom::TransformComponent>(entity);
			transform.position = { -150, -80, 100 };
			auto light = bloom::LightComponent{ bloom::SpotLight{
				{
					.color     = { 1, 1, 0 },
					.intensity = 5000
				},
				.innerCutoff = 0.15,
				.outerCutoff = 0.25,
				.radius = 5
			}};
			scene.addComponent(entity, light);
		}
		
		// Meshes
		{
			auto entity = scene.createEntity("Sphere");
			scene.addComponent(entity, sphereRenderer);
		}
		
		{
			auto const box1 = scene.createEntity("Box1");
			scene.addComponent(box1, cubeRenderer);
			auto& transform = scene.getComponent<bloom::TransformComponent>(box1);
			transform.position = { 200, 100, 50 };
			{
				auto box2 = scene.createEntity("Box2");
				scene.addComponent(box2, cubeRenderer);
				auto& transform = scene.getComponent<bloom::TransformComponent>(box2);
				transform.position = { 200, 300, -50 };
				
				scene.parent(box2, box1);
			}
			{
				auto box3 = scene.createEntity("Box3");
				scene.addComponent(box3, cubeRenderer);
				auto& transform = scene.getComponent<bloom::TransformComponent>(box3);
				transform.position = { -200, 300, -100 };
				
				scene.parent(box3, box1);
			}
		}
		
		
	}
	
	
}

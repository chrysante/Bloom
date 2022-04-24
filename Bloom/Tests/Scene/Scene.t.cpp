#include <Catch2/Catch2.hpp>

#include <iostream>

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components.hpp"

TEST_CASE() {
	return;
	bloom::Scene scene;
	
	auto mesh = bloom::StaticRenderMesh::create();
	
	bloom::MeshRenderComponent meshRenderer{ nullptr, mesh };
	
	auto entity = scene.createEntity("entity");
	scene.addComponent(entity, meshRenderer);
	
	std::cout << scene.hasComponent<bloom::TransformComponent>(entity) << std::endl;
	
//	auto view = scene.view<bloom::TransformComponent, bloom::MeshRenderComponent>();
//	view.each([&](auto const& transform, auto const& meshRenderer) {
//		std::cout << meshRenderer.mesh << std::endl;
//	});
	
	auto view = scene.view<bloom::MeshRenderComponent>();
	std::cout << view.size() << std::endl;
	view.each([&](auto const& m) {
		std::cout << m.mesh << std::endl;
	});
}

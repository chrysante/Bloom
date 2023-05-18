#include "SceneRenderer.hpp"

#include "Renderer.hpp"


#include "Bloom/Core/Core.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components/Lights.hpp"
#include "Bloom/Scene/Components/Transform.hpp"
#include "Bloom/Scene/Components/MeshRenderer.hpp"

#include <utl/functional.hpp>

namespace bloom {
	
	SceneRenderer::SceneRenderer(Renderer& renderer) {
		setRenderer(renderer);
	}
		
	void SceneRenderer::setRenderer(Renderer& renderer) {
		mRenderer = &renderer;
	}
	
	void SceneRenderer::draw(Scene const& scene,
							 Camera const& camera,
							 Framebuffer& framebuffer,
							 CommandQueue& commandQueue)
	{
		bloomExpect(mRenderer);
		std::array const scenes{ &scene };
		draw(scenes, camera, framebuffer, commandQueue);
	}
	
	void SceneRenderer::draw(std::span<Scene const* const> scenes,
							 Camera const& camera,
							 Framebuffer& framebuffer,
							 CommandQueue& commandQueue)
	{
		bloomExpect(mRenderer);
		renderer().beginScene(camera);
		
		for (auto* scene: scenes) {
			submitScene(*scene);
		}
		
		submitExtra();
		
		renderer().endScene();
		
		renderer().draw(framebuffer, commandQueue);
	}
	
	template <typename LightComponent>
	static void submitLights(Renderer& renderer, Scene const& scene, auto&& lightModifier) {
		auto view = scene.view<TransformMatrixComponent const, LightComponent const>();
		view.each([&](auto id, TransformMatrixComponent const& transform, LightComponent light) {
			lightModifier(transform, light);
			renderer.submit(light.light);
		});
	}

	template <typename Light>
	static void submitLights(Renderer& renderer, Scene const& scene) {
		submitLights<Light>(renderer, scene, utl::noop);
	}
	
	void SceneRenderer::submitScene(Scene const& scene) {
		/* submit meshes */ {
			auto view = scene.view<TransformMatrixComponent const, MeshRendererComponent const>();
			view.each([&](auto const id, TransformMatrixComponent const& transform, MeshRendererComponent const& meshRenderer) {
				if (!meshRenderer.mesh || !meshRenderer.materialInstance || !meshRenderer.materialInstance->material()) {
					return;
				}
				renderer().submit(meshRenderer.mesh->getRenderer(),
								  meshRenderer.materialInstance,
								  transform.matrix);
			});
		}
		
		submitLights<PointLightComponent>(renderer(), scene, [](auto& transform, auto& light) {
			light.light.position = transform.matrix.column(3).xyz;
		});
		submitLights<SpotLightComponent>(renderer(), scene, [](auto& transform, auto& light) {
			light.light.position = transform.matrix.column(3).xyz;
			light.light.direction = mtl::normalize((transform.matrix * mtl::float4{ 1, 0, 0, 0 }).xyz);
		});
		submitLights<DirectionalLightComponent>(renderer(), scene, [](auto& transform, auto& light) {
			light.light.direction = mtl::normalize((transform.matrix * mtl::float4{ 0, 0, 1, 0 }).xyz);
		});
		submitLights<SkyLightComponent>(renderer(), scene);
	}
	
}

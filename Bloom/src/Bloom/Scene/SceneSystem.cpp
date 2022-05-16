#include "SceneSystem.hpp"

#include "Scene.hpp"
#include "ScriptSystem.hpp"
#include "Components/AllComponents.hpp"

#include "Bloom/Core/Reference.hpp"
#include "Bloom/Application/Application.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"
#include "Bloom/Graphics/Renderer.hpp"

#include <utl/stack.hpp>

namespace bloom {
    
	SceneSystem::SceneSystem(): _scriptSystem(new ScriptSystem(this)) {
		
	}
	
	SceneSystem::~SceneSystem() {
		stopUpdateThread();
	}
	
	void SceneSystem::init() {
		_scriptSystem->init();
	}
	
	void SceneSystem::setScene(Reference<Scene> scene) {
		_scene = std::move(scene);
		_scriptSystem->onSceneConstruction();
	}
	
	void SceneSystem::runUpdateThread() {
		_isRunning = true;
		_updateThread = std::thread([this]{
			updateThread();
		});
	}
	
	void SceneSystem::stopUpdateThread() {
		if (!_isRunning) {
			return;
		}
		_isRunning = false;
		_updateThread.join();
	}
	
	template <typename LightComponent>
	static void submitLights(Renderer& renderer, Scene const* scene, auto&& lightModifier) {
		auto view = scene->view<TransformMatrixComponent const, LightComponent const>();
		view.each([&](auto id, TransformMatrixComponent const& transform, LightComponent light) {
			lightModifier(transform, light);
			renderer.submit(light.light);
		});
	}
	
	template <typename Light>
	static void submitLights(Renderer& renderer, Scene const* scene) {
		submitLights<Light>(renderer, scene, [](auto&, auto&) { /* no-op */ });
	}
	
	void SceneSystem::sumbitToRenderer(Renderer& renderer) {
		if (!_scene) {
			return;
		}
		auto* scene = _scene.get();
		/* submit meshes */ {
			auto view = scene->view<TransformMatrixComponent const, MeshRendererComponent const>();
			view.each([&](auto const id, TransformMatrixComponent const& transform, MeshRendererComponent const& meshRenderer) {
				if (!meshRenderer.mesh || !meshRenderer.material) {
					return;
				}
				EntityRenderData entityData;
				entityData.transform = transform.matrix;
				entityData.ID = utl::to_underlying(id);
				renderer.submit(meshRenderer.mesh->getRenderMesh(),
								 Reference<Material>(meshRenderer.material, &meshRenderer.material->material),
								 entityData);
			});
		}

		submitLights<PointLightComponent>(renderer, scene, [](auto& transform, auto& light) {
			light.light.position = transform.matrix.column(3).xyz;
		});
		submitLights<SpotLightComponent>(renderer, scene, [](auto& transform, auto& light) {
			light.light.position = transform.matrix.column(3).xyz;
			light.light.direction = mtl::normalize((transform.matrix * mtl::float4{ 1, 0, 0, 0 }).xyz);
		});
		submitLights<DirectionalLightComponent>(renderer, scene, [](auto& transform, auto& light) {
			light.light.direction = mtl::normalize((transform.matrix * mtl::float4{ 0, 0, 1, 0 }).xyz);
		});
		submitLights<SkyLightComponent>(renderer, scene);
	}
	
	void SceneSystem::applyTransformHierarchy() {
		auto view = _scene->view<TransformComponent const, TransformMatrixComponent>();
		view.each([&](auto const id, TransformComponent const& transform, TransformMatrixComponent& transformMatrix) {
			transformMatrix.matrix = transform.calculate();
		});
		
		utl::stack<EntityID> stack(_scene->gatherRoots());
		
		while (stack) {
			auto const current = stack.pop();
			auto const& currentTransform = _scene->getComponent<TransformMatrixComponent>(current);
			
			auto const children = _scene->gatherChildren(current);
			for (auto const c: children) {
				auto& childTransform = _scene->getComponent<TransformMatrixComponent>(c);
				childTransform.matrix = currentTransform.matrix * childTransform.matrix;
				stack.push(c);
			}
		}
	}
	
	Scene SceneSystem::copyScene() {
		return _scene->copy();
	}

	void SceneSystem::updateThread() {
		_beginTime = std::chrono::high_resolution_clock::now();
		
		while (_isRunning) {
			constexpr std::int64_t oneBillion = 1'000'000'000;
			std::unique_lock lock(_updateMutex);
			
			auto const stepBeginTime = std::chrono::high_resolution_clock::now();
			TimeStep const t = {
				(stepBeginTime - _beginTime).count() / (double)oneBillion,
				1.0 / _updateOptions.stepsPerSecond
			};
			

			updateLoop(t);
			
			// End
			auto const stepEndTime = std::chrono::high_resolution_clock::now();
			std::int64_t const stepDurationNS = (stepEndTime - stepBeginTime).count();
			std::int64_t const remaining = oneBillion / _updateOptions.stepsPerSecond - stepDurationNS;
			if (remaining < 0) {
				bloomLog(warning, "Failed to meet time step requirement of {}s", t.delta);
			}
			else {
				lock.unlock();
				std::this_thread::sleep_for(std::chrono::nanoseconds(remaining));
			}
		}
	}
	
	void SceneSystem::updateLoop(TimeStep t) {
				
		_scriptSystem->onSceneUpdate(t);
		
		
	}

}

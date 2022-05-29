#include "SceneSystem.hpp"

#include "Bloom/Scene/Scene.hpp"

#include <utl/stack.hpp>

namespace bloom {
	
	void SceneSystem::loadScene(Reference<Scene> scene) {
		auto const [_, success] = mScenes.insert({ scene->handle().id(), std::move(scene) });
		if (!success) {
			bloomLog(error, "Failed to load scene. Scene is already loaded.");
			return;
		}
		setPointers();
	}
	
	void SceneSystem::unloadScene(utl::UUID id) {
		auto const itr = mScenes.find(id);
		auto* const scene = itr->second.get();
		if (itr == mScenes.end()) {
			bloomLog(error, "Failed to unload scene. Scene was not loaded.");
			return;
		}
		mScenes.erase(itr);
		dispatch(DispatchToken::now, UnloadSceneEvent{ .scene = scene });
		setPointers();
	}
	
	void SceneSystem::unloadAll() {
		for (auto&& [key, scene]: mScenes) {
			dispatch(DispatchToken::now, UnloadSceneEvent{ .scene = scene.get() });
		}
		mScenes.clear();
		setPointers();
	}
	
	std::unique_lock<std::mutex> SceneSystem::lock() {
		return std::unique_lock(mMutex);
	}
	
	void SceneSystem::applyTransformHierarchy() {
		for (auto scene: scenes()) {
			auto view = scene->view<Transform const, TransformMatrixComponent>();
			view.each([&](auto const id, Transform const& transform, TransformMatrixComponent& transformMatrix) {
				transformMatrix.matrix = transform.calculate();
			});

			utl::stack<EntityID> stack(scene->gatherRoots());

			while (stack) {
				auto const current = stack.pop();
				auto const& currentTransform = scene->getComponent<TransformMatrixComponent>(current);

				auto const children = scene->gatherChildren(current);
				for (auto const c: children) {
					auto& childTransform = scene->getComponent<TransformMatrixComponent>(c);
					childTransform.matrix = currentTransform.matrix * childTransform.matrix;
					stack.push(c);
				}
			}
		}
	}
	
	void SceneSystem::start() {
		std::unique_lock lock(mMutex);
		mBackupScenes.clear();
		mBackupScenes.insert(mScenes.begin(), mScenes.end());
		
		mSimScenes.clear();
		mSimScenes.insert(mScenes.begin(), mScenes.end());
	}
	
	void SceneSystem::stop() {
		std::unique_lock lock(mMutex);
		mSimScenes.clear();
		mScenes.clear();
		mScenes.insert(mBackupScenes.begin(), mBackupScenes.end());
		setPointers();
	}
	
	void SceneSystem::pause() {
		
	}
	
	void SceneSystem::resume() {
		
	}
	
	void SceneSystem::step(Timestep) {
		tryCopyOut();
		
		if (mSimScenes.empty()) {
			return;
		}
		
		for (auto const& [id, scene]: mSimScenes) {
			for (auto&& [id, transform]: scene->view<Transform>().each()) {
				transform.position.x += 0.01;
				break;
			}
		}
	}
	
	void SceneSystem::tryCopyOut() {
		std::unique_lock lock(mMutex, std::try_to_lock);
		if (!lock) {
			return;
		}
		mScenes.clear();
		mScenes.insert(mSimScenes.begin(), mSimScenes.end());
		setPointers();
	}

	void SceneSystem::setPointers() {
		mScenePtrs.resize(mScenes.size(), utl::no_init);
		std::transform(mScenes.begin(), mScenes.end(),
					   mScenePtrs.begin(),
					   [](auto&& p) { return p.second.get(); });
	}
	
}

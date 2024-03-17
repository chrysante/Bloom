#include "Bloom/Runtime/SceneSystem.h"

#include <range/v3/view.hpp>
#include <utl/stack.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Runtime/ScriptSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Scene/SceneEvents.h"

using namespace bloom;

void SceneSystem::init() {
    listen([this](ScriptsDidLoadEvent) {
        for (auto* scene: scenes()) {
            application().coreSystems().scriptSystem().onScriptCompile(*scene);
        }
    });
}

void SceneSystem::loadScene(Reference<Scene> scene) {
    auto* raw = scene.get();
    auto [itr, success] =
        mScenes.insert({ scene->handle().ID(), std::move(scene) });
    if (!success) {
        Logger::Error("Failed to load scene. Scene is already loaded.");
        return;
    }
    dispatch(DispatchToken::Now, SceneLoadedEvent{ raw });
    setPointers(mScenes);
}

void SceneSystem::unloadScene(utl::uuid id) {
    auto itr = mScenes.find(id);
    auto* scene = itr->second.get();
    if (itr == mScenes.end()) {
        Logger::Error("Failed to unload scene. Scene was not loaded.");
        return;
    }
    mScenes.erase(itr);
    dispatch(DispatchToken::Now, UnloadSceneEvent{ .scene = scene });
    setPointers(mScenes);
}

void SceneSystem::unloadAll() {
    for (auto&& [key, scene]: mScenes) {
        dispatch(DispatchToken::Now, UnloadSceneEvent{ .scene = scene.get() });
    }
    mScenes.clear();
    setPointers(mScenes);
}

std::unique_lock<std::mutex> SceneSystem::lock() {
    return std::unique_lock(mMutex);
}

void SceneSystem::applyTransformHierarchy() {
    for (auto scene: scenes()) {
        auto view = scene->view<Transform const, TransformMatrixComponent>();
        view.each([&](auto /* ID */, Transform const& transform,
                      TransformMatrixComponent& transformMatrix) {
            transformMatrix.matrix = transform.calculate();
        });
        utl::stack<EntityID> stack(scene->gatherRoots());
        while (!stack.empty()) {
            auto current = stack.pop();
            auto& currentTransform =
                scene->getComponent<TransformMatrixComponent const>(current);

            auto children = scene->gatherChildren(current);
            for (auto c: children) {
                auto& childTransform =
                    scene->getComponent<TransformMatrixComponent>(c);
                childTransform.matrix =
                    currentTransform.matrix * childTransform.matrix;
                stack.push(c);
            }
        }
    }
}

void SceneSystem::start() {
    auto L = lock();
    mSimScenes = mScenes | ranges::views::transform([](auto& p) {
        return std::pair{ p.first, p.second->clone() };
    }) | ranges::to<utl::hashmap<utl::uuid, Reference<Scene>>>;
    setPointers(mSimScenes);
    auto& scriptSystem = application().coreSystems().scriptSystem();
    for (auto* scene: scenes()) {
        scriptSystem.onSceneInit(*scene);
    }
}

void SceneSystem::stop() {
    auto L = lock();
    mSimScenes.clear();
    setPointers(mScenes);
}

void SceneSystem::pause() {}

void SceneSystem::resume() {}

void SceneSystem::step(Timestep timestep) {
    auto L = lock();
    auto& scriptSystem = application().coreSystems().scriptSystem();
    for (auto* scene: scenes()) {
        scriptSystem.onSceneUpdate(*scene, timestep);
    }
}

void SceneSystem::setPointers(auto const& sceneMap) {
    mScenePtrs = sceneMap | ranges::views::transform([](auto&& p) {
        return p.second.get();
    }) | ranges::to<utl::small_vector<Scene*>>;
}

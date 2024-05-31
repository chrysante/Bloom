#include "Bloom/Runtime/SceneSystem.h"

#include <range/v3/view.hpp>
#include <utl/stack.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Runtime/PhysicsSystem.h"
#include "Bloom/Runtime/ScriptSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Scene/SceneEvents.h"

using namespace bloom;

void SceneSystem::init() {
    listen([this](ScriptsDidCompileEvent) {
        if (scene()) {
            application().coreSystems().scriptSystem().onScriptCompile(
                *scene());
        }
    });
}

void SceneSystem::loadScene(Reference<Scene> scene) {
    if (mScene.get() == scene.get()) {
        Logger::Warn("Scene is already loaded");
        return;
    }
    if (mScene) {
        unloadScene();
    }
    mScene = std::move(scene);
    dispatch(DispatchToken::Now, SceneLoadedEvent{ .scene = mScene.get() });
}

void SceneSystem::unloadScene() {
    if (!scene()) {
        Logger::Error("Failed to unload scene. No scene was loaded");
        return;
    }
    dispatch(DispatchToken::Now, UnloadSceneEvent{ .scene = scene() });
    mScene.reset();
}

std::unique_lock<std::mutex> SceneSystem::lock() {
    return std::unique_lock(mMutex);
}

void SceneSystem::start() {
    auto L = lock();
    mPlayScene = scene()->clone();
    auto& scriptSystem = application().coreSystems().scriptSystem();
    auto& physicsSystem = application().coreSystems().physicsSystem();
    physicsSystem.onSceneInit(*playScene(), playScene()->physicsParameters());
    scriptSystem.onSceneInit(*playScene());
}

void SceneSystem::stop() {
    auto& physicsSystem = application().coreSystems().physicsSystem();
    physicsSystem.onSceneStop();
    auto L = lock();
    mPlayScene.reset();
}

void SceneSystem::pause() {}

void SceneSystem::resume() {}

void SceneSystem::step(Timestep timestep) {
    auto L = lock();
    auto& scriptSystem = application().coreSystems().scriptSystem();
    auto& physicsSystem = application().coreSystems().physicsSystem();
    physicsSystem.update(*playScene(), timestep);
    scriptSystem.onSceneUpdate(*playScene(), timestep);
}

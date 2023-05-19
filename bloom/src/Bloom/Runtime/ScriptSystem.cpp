#include "Bloom/Runtime/ScriptSystem.hpp"

#include <utl/strcat.hpp>

#include "Bloom/Application/Application.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"
#include "Bloom/Scene/Components/Script.hpp"
#include "Bloom/Scene/Components/Transform.hpp"
#include "Bloom/Scene/Entity.hpp"
#include "Bloom/Scene/Scene.hpp"

using namespace bloom;

struct ScriptSystem::Impl {};

ScriptSystem::ScriptSystem(): impl(std::make_unique<Impl>()) {}

ScriptSystem::~ScriptSystem() = default;

void ScriptSystem::init() {
    listen([this](ScriptLoadEvent) { this->onScriptReload(); });
}

void ScriptSystem::onSceneConstruction() {
    //    auto& engine = *mEngine;
    //    initFn = engine.eval<std::function<void(ScriptObject&)>>("init");
    //    updateFn  = engine.eval<std::function<void(ScriptObject&,
    //    Timestep)>>("update"); renderFn =
    //    engine.eval<std::function<void(ScriptObject&)>>("render");

    forEach([&](EntityID id, ScriptComponent& script) {

    });
}

void ScriptSystem::onSceneInit() {
    forEach([&](ScriptComponent& script) {

    });
}

void ScriptSystem::onSceneUpdate(Timestep t) {
    forEach([&](ScriptComponent& script) {

    });
}

void ScriptSystem::onSceneRender() {
    forEach([&](ScriptComponent& script) {

    });
}

void ScriptSystem::onScriptReload() {
    auto& sceneSystem = application().coreSystems().sceneSystem();
}

void ScriptSystem::forEach(auto&& fn) {
#if 0
    auto& sceneSystem = application().coreSystems().sceneSystem();
    for (auto* scene: sceneSystem.scenes()) {
        scene->view<ScriptComponent>().each([&](auto const id, ScriptComponent& script) {
            fn(script);
        });
    }
#endif
}

#include "Bloom/Runtime/ScriptSystem.hpp"

#include <scatha/Sema/Entity.h>
#include <scatha/Sema/SymbolTable.h>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Application.hpp"
#include "Bloom/Asset/AssetManager.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"
#include "Bloom/Scene/Components/Script.hpp"
#include "Bloom/Scene/Components/Transform.hpp"
#include "Bloom/Scene/Entity.hpp"
#include "Bloom/Scene/Scene.hpp"

using namespace bloom;

struct ScriptSystem::Impl {
    // scatha::Program program;
    utl::hashmap<scatha::sema::StructType const*, std::string> typenameMap;
};

ScriptSystem::ScriptSystem(): impl(std::make_unique<Impl>()) {}

ScriptSystem::~ScriptSystem() = default;

void ScriptSystem::init() {
    listen([this](ScriptsWillLoadEvent) { this->scriptsWillLoad(); });
    listen([this](ScriptsDidLoadEvent) { this->scriptsDidLoad(); });
}

void ScriptSystem::onSceneConstruction() {
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

void ScriptSystem::scriptsWillLoad() {
    impl->typenameMap.clear();
    auto& assetManager = application().coreSystems().assetManager();
#if 0
    auto* program = assetManager.getProgram();
    if (!program) {
        return;
    }
    auto& sym = program->symbolTable();
    for (auto* type: sym.structDependencyOrder()) {
        impl->typenameMap[type] = std::string(type->name());
    }
#endif
}

void ScriptSystem::scriptsDidLoad() {
    auto& scriptSystem = application().coreSystems().scriptSystem();
    auto& assetManager = application().coreSystems().assetManager();
#if 0
    auto* program = assetManager.getProgram();
    if (!program) {
        return;
    }
    auto& sym = program->symbolTable();
    utl::hashmap<scatha::sema::StructureType const*,
                 scatha::sema::StructureType const*>
        typemap;
    for (auto [type, name]: impl->typenameMap) {
        auto* newType = sym.lookup<scatha::sema::StructureType>(name);
        typemap[type] = newType;
    }
    forEach([&](ScriptComponent& script) {
        auto* oldType = script.classType;
        auto* newType = typemap[oldType];
        script.classType = newType;
        if (!oldType || !newType) {
            script.object = nullptr;
            return;
        }
        if (newType->size() != script.classType->size()) {
            script.object = scriptSystem.instantiateObject(newType);
        }
    });
#endif
}

// void* ScriptSystem::instantiateObject(
//     scatha::sema::StructType const* classType) {
//     assert(classType);
//     return malloc(classType->size());
// }

void ScriptSystem::forEach(auto&& fn) {
    auto& sceneSystem = application().coreSystems().sceneSystem();
    for (auto* scene: sceneSystem.scenes()) {
        scene->view<ScriptComponent>().each(fn);
    }
}

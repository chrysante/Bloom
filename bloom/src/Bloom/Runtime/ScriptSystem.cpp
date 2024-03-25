#include "Bloom/Runtime/ScriptSystem.h"

#include <cstring>

#include <scatha/Invocation/Target.h>
#include <scatha/Sema/Entity.h>
#include <scatha/Sema/LifetimeMetadata.h>
#include <scatha/Sema/SymbolTable.h>
#include <svm/VirtualMachine.h>
#include <svm/VirtualMemory.h>
#include <utl/functional.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Runtime/ScriptBindings.h"
#include "Bloom/Scene/Components/Script.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Entity.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Scene/SceneEvents.h"

using namespace bloom;
using namespace vml;

template <typename... T>
    requires(std::is_standard_layout_v<T> && ...)
static auto toWordArray(T const&... args) {
    static constexpr size_t NumWords = (utl::ceil_divide(sizeof(T), 8) + ...);
    std::array<uint64_t, NumWords> data{};
    size_t i = 0;
    (
        [&] {
        std::memcpy(&data[i], &args, sizeof args);
        i += utl::ceil_divide(sizeof(T), 8);
    }(),
        ...);
    return data;
}

struct ScriptSystem::Impl {
    std::optional<scatha::Target> target;
    svm::VirtualMachine VM;
};

namespace bloom {

void ScriptSystem_setTarget(ScriptSystem::Impl& impl, scatha::Target target) {
    impl.target = std::move(target);
}

} // namespace bloom

ScriptSystem::ScriptSystem(): impl(std::make_unique<Impl>()) {}

ScriptSystem::~ScriptSystem() = default;

void ScriptSystem::init() {
    listen([this](ScriptsWillLoadEvent) { scriptsWillCompile(); });
    listen([this](ScriptsDidLoadEvent) { scriptsDidCompile(); });
    listen([this](SceneLoadedEvent event) { onScriptCompile(*event.scene); });
}

static uint64_t allocateObject(svm::VirtualMachine& VM,
                               scatha::sema::StructType const* type,
                               EntityHandle handle) {
    auto addr = VM.allocateMemory(type->size(), type->align());
    auto bitAddr = std::bit_cast<uint64_t>(addr);
    auto ctors = type->findFunctions("new");
    if (ctors.empty()) {
        std::memset(VM.derefPointer(addr, type->size()), 0, type->size());
        return bitAddr;
    }
    if (ctors.size() > 1) {
        Logger::Warn(
            "'", type->name(),
            "' has multiple constructors; selecting unspecified constructor");
    }
    VM.execute(ctors.front()->binaryAddress().value(),
               toWordArray(bitAddr, ScriptEntityHandle::make(handle)));
    return bitAddr;
    /// More rigorous implementation, but we're hacking around right now
#if 0
    using enum scatha::sema::LifetimeOperation::Kind;
    switch (ctor.kind()) {
    case Trivial:
        std::memset(VM.derefPointer(addr, type->size()), 0, type->size());
        return bitAddr;
    case Nontrivial:
        
    default:
        Logger::Error("Cannot instantiate type without default constructor");
        return 0;
    }
#endif
}

void ScriptSystem::onScriptCompile(Scene& scene) {
    scene.view<ScriptComponent, ScriptPreservedData const>().each(
        [&](ScriptComponent& component, ScriptPreservedData const& data) {
        deserializeScript(component, data);
    });
}

void ScriptSystem::deserializeScript(ScriptComponent& component,
                                     ScriptPreservedData const& preservedData) {
    if (!impl->target) {
        return;
    }
    auto& target = *impl->target;
    auto types = target.symbolTable().globalScope().findEntities(
        preservedData.classname);
    if (types.empty()) {
        /// Set all pointers to null
        component = ScriptComponent{};
        return;
    }
    auto* type = dyncast<scatha::sema::StructType const*>(types.front());
    assignType(component, type);
}

void ScriptSystem::onSceneConstruction(Scene&) {}

static void runGuarded(auto&& f) {
    try {
        f();
    }
    catch (svm::RuntimeException const& e) {
        Logger::Error("Script execution error: ", e.what());
    }
}

void ScriptSystem::onSceneInit(Scene& scene) {
    if (!impl->target) {
        return;
    }
    scene.view<ScriptComponent>().each(
        [&](EntityID ID, ScriptComponent& component) {
        if (!component.type) {
            return;
        }
        runGuarded([&] {
            component.objectAddress = allocateObject(impl->VM, component.type,
                                                     EntityHandle(ID, &scene));
        });
    });
}

void ScriptSystem::onSceneUpdate(Scene& scene, Timestep timestep) {
    scene.view<ScriptComponent>().each([&](ScriptComponent& component) {
        if (!component.objectAddress || !component.updateFunction) {
            return;
        }
        runGuarded([&] {
            impl->VM.execute(component.updateFunction->binaryAddress().value(),
                             toWordArray(component.objectAddress, timestep));
        });
    });
}

void ScriptSystem::onSceneRender(Scene&) {}

scatha::sema::SymbolTable const* ScriptSystem::symbolTable() const {
    if (!impl->target) {
        return nullptr;
    }
    return &impl->target->symbolTable();
}

void ScriptSystem::scriptsWillCompile() { impl->VM.reset(); }

void ScriptSystem::scriptsDidCompile() {
    if (!impl->target) {
        return;
    }
    auto& target = *impl->target;
    impl->VM.reset();
    impl->VM.loadBinary(target.binary().data());
}

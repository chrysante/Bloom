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
using namespace scatha;

template <typename... T>
    requires(std::is_standard_layout_v<T> && ...)
static auto toWordArray(T const&... args) {
    static constexpr size_t NumWords = (utl::ceil_divide(sizeof(T), 8) + ...);
    std::array<uint64_t, NumWords> data{};
    size_t i = 0;
    ([&] {
        std::memcpy(&data[i], &args, sizeof args);
        i += utl::ceil_divide(sizeof(T), 8);
    }(), ...);
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
    listen([this](ScriptsWillCompileEvent) { scriptsWillCompile(); });
    listen([this](ScriptsDidCompileEvent) { scriptsDidCompile(); });
    listen([this](SceneLoadedEvent event) { onScriptCompile(*event.scene); });
}

static sema::Function const* selectConstructor(sema::StructType const* type) {
    /// We are looking for a constructor that takes a `&this` or `&mut this` and
    /// a `Bloom.Entity`
    for (auto* ctor: type->constructors()) {
        if (ctor->argumentCount() != 2) {
            continue;
        }
        auto* firstArg =
            scatha::dyncast<sema::ReferenceType const*>(ctor->argumentType(0));
        if (!firstArg || firstArg->base().get() != type) {
            continue;
        }
        auto* secondArg =
            scatha::dyncast<sema::StructType const*>(ctor->argumentType(1));
        if (!secondArg || (secondArg->name() != "Entity")) {
            continue;
        }
        auto* library =
            scatha::dyncast<sema::NativeLibrary const*>(secondArg->parent());
        if (!library || library->name() != "Bloom") {
            continue;
        }
        return ctor;
    }
    return nullptr;
}

/// Allocates and constructs an object of type \p type in the virtual address
/// space of \p VM
static svm::VirtualPointer allocateObject(svm::VirtualMachine& VM,
                                          sema::StructType const* type,
                                          EntityHandle handle) {
    auto* ctor = selectConstructor(type);
    if (!ctor) {
        Logger::Warn("Failed to find suitable constructor for object of type: ",
                     type->name());
        return {};
    }
    auto addr = VM.allocateMemory(type->size(), type->align());
    auto ctors = type->findFunctions("new");
    if (ctors.empty() && type->hasTrivialLifetime()) {
        std::memset(VM.derefPointer(addr, type->size()), 0, type->size());
        return addr;
    }
    VM.execute(ctor->binaryAddress().value(),
               toWordArray(addr, ScriptEntityHandle::make(handle)));
    return addr;
}

/// Destroys and deallocates the object at address \p addr
/// When the virtual machine is destroyed, all memory is freed even without
/// explicitly deallocating each object
static void deallocateObject(svm::VirtualMachine& VM,
                             sema::StructType const* type,
                             svm::VirtualPointer addr) {
    VM.deallocateMemory(addr, type->size(), type->align());
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
    auto* type = scatha::dyncast<sema::StructType const*>(types.front());
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
            auto addr = allocateObject(impl->VM, component.type,
                                       EntityHandle(ID, &scene));
            component.objectAddress = std::bit_cast<uint64_t>(addr);
        });
    });
}

void ScriptSystem::onSceneUpdate(Scene& scene, Timestep timestep) {
    scene.view<ScriptComponent>().each([&](ScriptComponent& component) {
        if (component.objectAddress == 0 || !component.updateFunction) {
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

void ScriptSystem::scriptsWillCompile() { impl->VM = {}; }

void ScriptSystem::scriptsDidCompile() {
    if (!impl->target) {
        return;
    }
    auto& target = *impl->target;
    impl->VM.reset();
    impl->VM.loadBinary(target.binary().data());
}

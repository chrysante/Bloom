#include "Bloom/Runtime/ScriptBindings.h"

#include <sstream>

#include <magic_enum.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/Input.h"
#include "Bloom/Runtime/PhysicsSystem.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Scene.h"

using namespace bloom;
using namespace vml;

static_assert(sizeof(ScriptTransform) == 10 * sizeof(double));
static_assert(alignof(ScriptTransform) == alignof(double));
static_assert(sizeof(ScriptEntityHandle) == 16);

#define API_EXPORT __attribute__((visibility("default")))

API_EXPORT extern "C" ScriptTransform bloomGetEntityTransform(
    ScriptEntityHandle e) {
    EntityHandle entity(e);
    auto transform = entity.get<Transform const>();
    auto t = ScriptTransform::make(transform);
    return t;
}

API_EXPORT extern "C" void bloomSetEntityTransform(ScriptEntityHandle entity,
                                                   ScriptTransform source) {
    Transform transform(source);
    EntityHandle(entity).get<Transform>() = transform;
    auto* physics = entity.native().tryGet<PhysicsMovableComponent>();
    if (!physics) {
        return;
    }
    auto& physicsSystem = Application::StaticInstance().physicsSystem();
    physicsSystem.setTransform(physics->bodyID, transform);
}

API_EXPORT extern "C" bool bloomIsKeyDown(uint64_t key) {
    return Application::StaticInstance().input().keyDown((Key)key);
}

std::string bloom::makeKeyTableSource() {
    std::stringstream source;
    for (Key key: magic_enum::enum_values<Key>()) {
        source << "public let Key_" << magic_enum::enum_name(key)
               << ": u64 = " << (uint64_t)key << ";\n";
    }
    return std::move(source).str();
}

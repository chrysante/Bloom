#include "Bloom/Runtime/ScriptBindings.h"

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
}

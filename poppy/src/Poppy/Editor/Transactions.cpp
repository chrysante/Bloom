#include "Poppy/Editor/Transactions.h"

#include "Bloom/Scene/Scene.h"

using namespace poppy;
using namespace bloom;

static bool applyComponent(EntityHandle entity,
                           EditEntityComponent::Variant const& v) {
    return std::visit([&]<typename T>(T const& value) {
        if constexpr (DataComponent<T>) {
            if (auto* component = entity.tryGet<T>()) {
                *component = value;
                return true;
            }
        }
        return false;
    }, v);
}

bool EditEntityComponent::commit() const {
    if (auto entity = handle().lock()) {
        return applyComponent(entity, _newValue);
    }
    return false;
}

bool EditEntityComponent::undo() const {
    if (auto entity = handle().lock()) {
        return applyComponent(entity, _oldValue);
    }
    return false;
}

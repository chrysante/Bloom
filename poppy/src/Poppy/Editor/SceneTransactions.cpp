#include "Poppy/Editor/SceneTransactions.h"

#include <imgui.h>

#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/TransactionManager.h"

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

EditEntityComponent::EditEntityComponent(WeakHandle handle, Variant oldValue,
                                         Variant newValue):
    EntityTransaction(std::move(handle)),
    _oldValue(std::move(oldValue)),
    _newValue(std::move(newValue)) {
    BL_ASSERT(_oldValue.index() == _newValue.index(),
              "Components must be of same type");
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

void poppy::editComponent(TransactionManager& transactionManager,
                          EntityHandle entity,
                          EditEntityComponent::Variant const& component,
                          utl::function_view<void()> uiFunction) {
    static EditEntityComponent::Variant backup;
    if (!ImGui::IsAnyItemActive()) {
        backup = component;
    }
    uiFunction();
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transactionManager.commit(
            new EditEntityComponent(entity.strongHandle(), backup, component));
    }
}

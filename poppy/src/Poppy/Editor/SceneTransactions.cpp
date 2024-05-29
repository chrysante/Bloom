#include "Poppy/Editor/SceneTransactions.h"

#include <imgui.h>

#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/TransactionManager.h"

using namespace poppy;
using namespace bloom;

using WeakHandle = EntityTransaction::WeakHandle;

static bool apply(WeakHandle const& handle, auto fn) {
    if (auto entity = handle.lock()) {
        return std::invoke(fn, entity);
    }
    return false;
}

bool CustomEntityTransaction::commit() const {
    return apply(handle(),
                 [&](EntityHandle entity) { return _commit(entity); });
}

bool CustomEntityTransaction::undo() const {
    return apply(handle(), [&](EntityHandle entity) { return _undo(entity); });
}

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
    return apply(handle(), [&](EntityHandle entity) {
        return applyComponent(entity, _newValue);
    });
}

bool EditEntityComponent::undo() const {
    return apply(handle(), [&](EntityHandle entity) {
        return applyComponent(entity, _oldValue);
    });
}

void poppy::editEntity(TransactionManager& transactionManager,
                       EntityHandle entity,
                       utl::function_view<void()> uiFunction,
                       std::function<bool(EntityHandle)> commit,
                       std::function<bool(EntityHandle)> undo) {
    uiFunction();
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transactionManager.commit(
            new CustomEntityTransaction(entity.strongHandle(),
                                        std::move(commit), std::move(undo)));
    }
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

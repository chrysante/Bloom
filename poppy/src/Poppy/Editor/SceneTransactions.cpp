#include "Poppy/Editor/SceneTransactions.h"

#include <imgui.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/TransactionManager.h"

using namespace poppy;
using namespace bloom;
using namespace ranges::views;

using WeakHandle = EntityTransaction::WeakHandle;

static bool apply(std::span<WeakHandle const> handles, auto&& components,
                  auto fn) {
    bool success = true;
    for (auto [handle, component]: zip(handles, components)) {
        if (auto entity = handle.lock()) {
            success &= std::invoke(fn, entity, component);
        }
        else {
            success = false;
        }
    }
    return success;
}

bool CustomEntityTransaction::commit() const {
    return apply(handles(), single(0),
                 [&](EntityHandle entity, auto) { return _commit(entity); });
}

bool CustomEntityTransaction::undo() const {
    return apply(handles(), single(0),
                 [&](EntityHandle entity, auto) { return _undo(entity); });
}

static bool applyComponent(EntityHandle entity,
                           EditEntityComponent::WeakVariant const& v) {
    return std::visit([&]<typename Weak>(Weak const& value) {
        using Strong = typename ToStrongComponent<Weak>::type;
        if constexpr (DataComponent<Strong>) {
            if (auto* component = entity.tryGet<Strong>()) {
                *component = value;
                return true;
            }
        }
        return false;
    }, v);
}

EditEntityComponent::EditEntityComponent(
    utl::small_vector<WeakHandle> handles,
    utl::small_vector<WeakVariant> oldValues,
    utl::small_vector<WeakVariant> newValues):
    EntityTransaction(std::move(handles)),
    _oldValues(std::move(oldValues)),
    _newValues(std::move(newValues)) {
    BL_ASSERT(ranges::all_of(zip(_oldValues, _newValues),
                             [](auto p) {
        return p.first.index() == p.second.index();
    }),
              "Components must be of same type");
}

bool EditEntityComponent::commit() const {
    return apply(handles(), _newValues,
                 [&](EntityHandle entity, auto const& newValue) {
        return applyComponent(entity, newValue);
    });
}

bool EditEntityComponent::undo() const {
    return apply(handles(), _oldValues,
                 [&](EntityHandle entity, auto const& oldValue) {
        return applyComponent(entity, oldValue);
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

static EditEntityComponent::WeakVariant dereference(
    EditEntityComponent::ConstPtrVariant p) {
    return std::visit([](auto* p) -> EditEntityComponent::WeakVariant {
        return *p;
    }, p);
}

void poppy::editComponents(
    TransactionManager& transactionManager,
    std::span<EntityHandle const> entities,
    std::span<EditEntityComponent::ConstPtrVariant const> components,
    utl::function_view<void(TAToken&)> uiFunction) {
    static utl::small_vector<EditEntityComponent::WeakVariant> backups;
    if (!ImGui::IsAnyItemActive()) {
        backups = components | transform(dereference) | ToSmallVector<>;
    }
    TAToken token;
    uiFunction(token);
    if (backups.empty()) {
        return;
    }
    if (token.state() == TAToken::Cancelled) {
        for (auto [entity, component, backup]:
             zip(entities, components, backups))
        {
            visitComponentType(ComponentKind(component.index()),
                               [entity = &entity,
                                backup = &backup]<typename T>(utl::tag<T>) {
                using Weak = typename ToWeakComponent<T>::type;
                entity->get<T>() = std::get<Weak>(*backup);
            });
        }
    }
    else if (token.state() == TAToken::Committed ||
             ImGui::IsItemDeactivatedAfterEdit())
    {
        auto* TA = new EditEntityComponent{
            entities | transform(&EntityHandle::strongHandle), backups,
            components | transform(dereference) | ToSmallVector<>
        };
        transactionManager.commit(TA);
    }
    if (!ImGui::IsAnyItemActive()) {
        backups.clear();
    }
}

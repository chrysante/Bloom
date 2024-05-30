#ifndef POPPY_EDITOR_SCENETRANSACTIONS_H
#define POPPY_EDITOR_SCENETRANSACTIONS_H

#include <functional>
#include <span>
#include <variant>

#include <utl/function_view.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Core/Utils.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Bloom/Scene/Fwd.h"
#include "Poppy/Editor/Transaction.h"

namespace poppy {

class TransactionManager;
class EntitySelectionManager;

class CopyEntitiesTransaction: public Transaction {
public:
    explicit CopyEntitiesTransaction(
        bloom::WeakReference<bloom::Scene> scene,
        std::span<bloom::EntityHandle const> entities,
        EntitySelectionManager& selectionManager,
        std::function<void(bloom::EntityHandle, size_t index)> postProcess);

    bool commit() const override;

    bool undo() const override;

private:
    bloom::WeakReference<bloom::Scene> weakScene;
    utl::small_vector<utl::uuid> originalIDs;
    utl::small_vector<utl::uuid> cloneIDs;
    EntitySelectionManager& selectionManager;
    std::function<void(bloom::EntityHandle, size_t index)> postProcess;
};

/// A transaction involving a set of entities
class EntityTransaction: public Transaction {
public:
    struct WeakHandle {
        WeakHandle(bloom::StrongEntityHandle const& handle):
            scene(handle.scene()), ID(handle.ID()) {}

        WeakHandle(bloom::EntityHandle handle);

        bloom::StrongEntityHandle lock() const {
            auto s = scene.lock();
            auto validID = s ? ID : bloom::EntityID{};
            return bloom::StrongEntityHandle(validID, std::move(s));
        }

        bloom::WeakReference<bloom::Scene> scene;
        bloom::EntityID ID;
    };

    WeakHandle const& singleHandle() const {
        BL_ASSERT(_handles.size() == 1);
        return _handles.front();
    }

    std::span<WeakHandle const> handles() const { return _handles; }

    WeakHandle const& handleAt(size_t index) const { return _handles[index]; }

protected:
    explicit EntityTransaction(utl::small_vector<WeakHandle> handles):
        _handles(std::move(handles)) {}

private:
    utl::small_vector<WeakHandle> _handles;
};

class CustomEntityTransaction: public EntityTransaction {
public:
    explicit CustomEntityTransaction(
        WeakHandle handle, std::invocable<bloom::EntityHandle> auto commit,
        std::invocable<bloom::EntityHandle> auto undo):
        EntityTransaction({ std::move(handle) }),
        _commit([commit = std::move(commit)](auto entities) {
            return commit(entities.front());
        }),
        _undo([undo = std::move(undo)](auto entities) {
            return undo(entities.front());
        }) {}

    explicit CustomEntityTransaction(
        utl::small_vector<WeakHandle> handles,
        std::function<bool(std::span<bloom::EntityHandle const>)> commit,
        std::function<bool(std::span<bloom::EntityHandle const>)> undo):
        EntityTransaction(std::move(handles)),
        _commit(std::move(commit)),
        _undo(std::move(undo)) {}

private:
    bool commit() const override;
    bool undo() const override;

    std::function<bool(std::span<bloom::EntityHandle const>)> _commit;
    std::function<bool(std::span<bloom::EntityHandle const>)> _undo;
};

/// Modifies components of a set of entities
class EditEntityComponent: public EntityTransaction {
public:
    using Variant =
        bloom::CopyTypeParameters<bloom::ComponentList, std::variant>;

    using WeakVariant =
        bloom::CopyTypeParameters<bloom::ComponentList, std::variant,
                                  bloom::ToWeakComponent>;

    using ConstPtrVariant = bloom::CopyTypeParameters<
        bloom::ComponentList, std::variant,
        bloom::Compose<std::add_pointer, std::add_const>::impl>;

    explicit EditEntityComponent(utl::small_vector<WeakHandle> handles,
                                 utl::small_vector<WeakVariant> oldValues,
                                 utl::small_vector<WeakVariant> newValues);

private:
    bool commit() const override;
    bool undo() const override;

    utl::small_vector<WeakVariant> _oldValues;
    utl::small_vector<WeakVariant> _newValues;
};

struct TAToken {
    enum State { None, Committed, Cancelled };

    /// Resets the state to that before the edit started
    void cancel() { _state = Cancelled; }

    /// Explicitly commits the current edit. If the edit is done by a widget
    /// after whose use `ImGui::IsItemDeactivatedAfterEdit()` returns `true`,
    /// this does not need to be called
    void commit() { _state = Committed; }

    ///
    State state() const { return _state; }

private:
    State _state = None;
};

/// ImGui wrapper that will commit the modification in \p uiFunction to \p
/// transactionManager
void editEntity(TransactionManager& transactionManager,
                bloom::EntityHandle entity,
                utl::function_view<void()> uiFunction,
                std::function<bool(bloom::EntityHandle)> commit,
                std::function<bool(bloom::EntityHandle)> undo);

/// ImGui wrapper that will commit the modification in \p uiFunction to \p
/// transactionManager
void editComponents(
    TransactionManager& transactionManager,
    std::span<bloom::EntityHandle const> entities,
    std::span<EditEntityComponent::ConstPtrVariant const> components,
    utl::function_view<void(TAToken&)> uiFunction);

} // namespace poppy

#endif // POPPY_EDITOR_SCENETRANSACTIONS_H

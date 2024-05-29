#ifndef POPPY_EDITOR_SCENETRANSACTIONS_H
#define POPPY_EDITOR_SCENETRANSACTIONS_H

#include <functional>
#include <variant>

#include <utl/function_view.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Core/Utils.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Bloom/Scene/Fwd.h"
#include "Poppy/Editor/Transaction.h"

namespace poppy {

class TransactionManager;

/// A transaction involving a single entity
class EntityTransaction: public Transaction {
public:
    struct WeakHandle {
        WeakHandle(bloom::StrongEntityHandle const& handle):
            scene(handle.scene()), ID(handle.ID()) {}

        bloom::StrongEntityHandle lock() const {
            auto s = scene.lock();
            auto validID = s ? ID : bloom::EntityID{};
            return bloom::StrongEntityHandle(validID, std::move(s));
        }

        bloom::WeakReference<bloom::Scene> scene;
        bloom::EntityID ID;
    };

    WeakHandle const& handle() const { return _handle; }

protected:
    explicit EntityTransaction(WeakHandle handle): _handle(std::move(handle)) {}

private:
    WeakHandle _handle;
};

class CustomEntityTransaction: public EntityTransaction {
public:
    explicit CustomEntityTransaction(
        WeakHandle handle, std::function<bool(bloom::EntityHandle)> commit,
        std::function<bool(bloom::EntityHandle)> undo):
        EntityTransaction(std::move(handle)),
        _commit(std::move(commit)),
        _undo(std::move(undo)) {}

private:
    bool commit() const override;
    bool undo() const override;

    std::function<bool(bloom::EntityHandle)> _commit;
    std::function<bool(bloom::EntityHandle)> _undo;
};

/// Modifies a component of an entity
class EditEntityComponent: public EntityTransaction {
public:
    using Variant =
        bloom::CopyTypeParameters<bloom::ComponentList, std::variant>;

    explicit EditEntityComponent(WeakHandle handle, Variant oldValue,
                                 Variant newValue);

private:
    bool commit() const override;
    bool undo() const override;

    Variant _oldValue;
    Variant _newValue;
};

struct CancelToken {
    void cancel() { _cancelled = true; }
    bool cancelled() const { return _cancelled; }

private:
    bool _cancelled = false;
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
void editComponent(TransactionManager& transactionManager,
                   bloom::EntityHandle entity,
                   EditEntityComponent::Variant const& component,
                   utl::function_view<void(CancelToken&)> uiFunction);

} // namespace poppy

#endif // POPPY_EDITOR_SCENETRANSACTIONS_H

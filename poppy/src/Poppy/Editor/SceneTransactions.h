#ifndef POPPY_EDITOR_SCENETRANSACTIONS_H
#define POPPY_EDITOR_SCENETRANSACTIONS_H

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

/// Modifies a component of an entity
class EditEntityComponent: public EntityTransaction {
public:
    using Variant =
        bloom::CopyTypeParameters<bloom::ComponentList, std::variant>;

    explicit EditEntityComponent(WeakHandle handle, Variant oldValue,
                                 Variant newValue);

    bool commit() const;

    bool undo() const;

private:
    Variant _oldValue;
    Variant _newValue;
};

/// ImGui wrapper that will commit the modification in \p uiFunction to \p
/// transactionManager
void editComponent(TransactionManager& transactionManager,
                   bloom::EntityHandle entity,
                   EditEntityComponent::Variant const& component,
                   utl::function_view<void()> uiFunction);

} // namespace poppy

#endif // POPPY_EDITOR_SCENETRANSACTIONS_H

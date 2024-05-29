#ifndef POPPY_EDITOR_TRANSACTIONS_H
#define POPPY_EDITOR_TRANSACTIONS_H

#include <variant>

#include "Bloom/Core/Reference.h"
#include "Bloom/Core/Utils.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Bloom/Scene/Fwd.h"

namespace poppy {

///
class Transaction {
public:
    virtual ~Transaction() = default;

private:
    friend class TransactionManager;

    virtual bool commit() const = 0;

    virtual bool undo() const = 0;
};

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

class EntityComponentTransaction: public EntityTransaction {
public:
    using EntityTransaction::EntityTransaction;
};

class EditEntityComponent: public EntityComponentTransaction {
public:
    using Variant =
        bloom::CopyTypeParameters<bloom::ComponentList, std::variant>;

    template <bloom::DataComponent T>
    explicit EditEntityComponent(WeakHandle handle, T oldValue, T newValue):
        EntityComponentTransaction(std::move(handle)),
        _oldValue(std::move(oldValue)),
        _newValue(std::move(newValue)) {}

    bool commit() const;

    bool undo() const;

private:
    Variant _oldValue;
    Variant _newValue;
};

} // namespace poppy

#endif // POPPY_EDITOR_TRANSACTIONS_H

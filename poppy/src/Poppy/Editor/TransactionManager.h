#ifndef POPPY_EDITOR_TRANSACTIONMANAGER_H
#define POPPY_EDITOR_TRANSACTIONMANAGER_H

#include <functional>
#include <memory>

namespace poppy {

class Transaction;

/// Undo/redo system
class TransactionManager {
public:
    TransactionManager();

    ~TransactionManager();

    ///
    void commit(std::unique_ptr<Transaction> transaction);

    /// Takes ownership of the transaction
    void commit(Transaction* transaction);

    ///
    void commit(std::function<bool()> action, std::function<bool()> undo);

    /// Undoes the last transaction
    /// \Returns `true` if there was a last transaction and that transaction was
    /// successfully undone
    bool undo();

    /// Redoes the last undo
    /// \Returns `true` if there was a transaction to redo and that transaction
    /// was successfully redone
    bool redo();

    struct Data;

private:
    std::unique_ptr<Data> data;
};

} // namespace poppy

#endif // POPPY_EDITOR_TRANSACTIONMANAGER_H

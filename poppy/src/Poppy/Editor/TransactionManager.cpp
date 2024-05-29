#include "Poppy/Editor/TransactionManager.h"

#include <vector>

#include "Bloom/Core/Debug.h"
#include "Poppy/Editor/Transactions.h"

using namespace poppy;
using namespace bloom;

namespace {

struct CustomTransaction: Transaction {
    CustomTransaction(std::function<bool()> action,
                      std::function<bool()> undoAction):
        action(std::move(action)), undoAction(std::move(undoAction)) {}

    bool commit() const override { return action(); }

    bool undo() const override { return undoAction(); }

    std::function<bool()> action;
    std::function<bool()> undoAction;
};

class TransactionStack {
public:
    void push(std::unique_ptr<Transaction> transaction) {
        elements.erase(elements.begin() + topEnd, elements.end());
        elements.push_back(std::move(transaction));
        ++topEnd;
    }

    Transaction* pop() {
        if (topEnd == 0) {
            return nullptr;
        }
        BL_ASSERT(topEnd <= elements.size());
        return elements[--topEnd].get();
    }

    Transaction* unpop() {
        if (topEnd == elements.size()) {
            return nullptr;
        }
        BL_ASSERT(topEnd < elements.size());
        return elements[topEnd++].get();
    }

private:
    using Array = std::vector<std::unique_ptr<Transaction>>;

    /// The index of the element one past the last committed transaction
    size_t topEnd = 0;
    Array elements;
};

} // namespace

using Data = TransactionManager::Data;

struct TransactionManager::Data {
    TransactionStack stack;
};

TransactionManager::TransactionManager(): data(std::make_unique<Data>()) {}

TransactionManager::~TransactionManager() = default;

void TransactionManager::commit(std::unique_ptr<Transaction> transaction) {
    if (!transaction->commit()) {
        Logger::Error("Failed to commit transaction");
    }
    data->stack.push(std::move(transaction));
}

void TransactionManager::commit(Transaction* transaction) {
    commit(std::unique_ptr<Transaction>(transaction));
}

void TransactionManager::commit(std::function<bool()> action,
                                std::function<bool()> undo) {
    commit(new CustomTransaction{ std::move(action), std::move(undo) });
}

bool TransactionManager::undo() {
    auto* transaction = data->stack.pop();
    if (!transaction) {
        return false;
    }
    return transaction->undo();
}

bool TransactionManager::redo() {
    auto* transaction = data->stack.unpop();
    if (!transaction) {
        return false;
    }
    return transaction->commit();
}

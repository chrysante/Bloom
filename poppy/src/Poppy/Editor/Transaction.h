#ifndef POPPY_EDITOR_TRANSACTION_H
#define POPPY_EDITOR_TRANSACTION_H

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

} // namespace poppy

#endif // POPPY_EDITOR_TRANSACTION_H

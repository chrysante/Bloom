#ifndef BLOOM_MATERIALCOMPILER_VALUES_H
#define BLOOM_MATERIALCOMPILER_VALUES_H

#include <span>
#include <string>
#include <vector>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/ilist.hpp>
#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/MaterialCompiler/Fwd.h"

#define DECLARE_TYPE(TYPE)                                                     \
    template <typename T = TYPE>                                               \
    T const* type() const {                                                    \
        return cast<T const*>(Value::type());                                  \
    }

#define DECLARE_OPERAND(TYPE, NAME, CAP_NAME, INDEX)                           \
    template <typename T = TYPE>                                               \
    T* NAME() const {                                                          \
        return cast<T*>(User::operand(INDEX));                                 \
    }                                                                          \
    void set##CAP_NAME(Value* value) { User::setOperand(INDEX, value); }

#define DECLARE_OPERAND_RANGE(TYPE, NAME, CAP_NAME, INDEX_OFFSET)              \
    template <typename T = TYPE>                                               \
    auto NAME##s() const {                                                     \
        return User::operands().subspan(INDEX_OFFSET) |                        \
               ranges::views::transform(cast<T*>);                             \
    }                                                                          \
    void set##CAP_NAME(size_t index, Value* value) {                           \
        User::setOperand(index + INDEX_OFFSET, value);                         \
    }

namespace bloom::matc {

template <typename T>
class DynAllocator: public std::allocator<T> {
public:
    void destroy(T* ptr) { utl::dyn_delete(ptr); }

    void deallocate(T* ptr, size_t count) { ::operator delete(ptr, count); }
};

template <typename T>
using List = utl::ilist<T, DynAllocator<T>>;

template <typename T>
using ListNode = utl::ilist_node<T>;

class Value {
public:
    ~Value();

    Type const* type() const { return _type; }

    std::string const& name() const { return _name; }

    auto users() const { return _users.values() | ranges::views::keys; }

    void detachAllUsers();

protected:
    Value(ValueNodeType nodeType, std::string name, Type const* type):
        _nodeType(nodeType), _name(std::move(name)), _type(type) {}

private:
    friend ValueNodeType dyncast_get_type(Value const& value) {
        return value._nodeType;
    }

    friend class User;
    void addUser(User* user);
    void removeUser(User* user);

    ValueNodeType _nodeType;
    std::string _name;
    Type const* _type;
    utl::hashmap<User*, int> _users;
};

class FunctionParameter: public Value, public ListNode<FunctionParameter> {
public:
    explicit FunctionParameter(std::string name, Type const* type):
        Value(ValueNodeType::FunctionParameter, std::move(name), type) {}
};

class User: public Value {
public:
    std::span<Value* const> operands() const { return _operands; }

    size_t numOperands() const { return operands().size(); }

    Value* operand(size_t index) const {
        assert(index < _operands.size());
        return _operands[index];
    }

    void setOperand(size_t index, Value* value);

protected:
    User(ValueNodeType nodeType, std::string name, Type const* type,
         std::vector<Value*> operands = {}):
        Value(nodeType, std::move(name), type),
        _operands(std::move(operands)) {}

private:
    friend class Value;
    void killOperand(Value* operand);

    std::vector<Value*> _operands;
};

class Constant: public User {
protected:
    using User::User;
};

class VectorConstant: public Constant {
public:
    explicit VectorConstant(Context& ctx, std::string name, int numComponents,
                            vml::float4 value);

    DECLARE_TYPE(VectorType)

    int numComponents() const { return numc; }

    vml::float4 get() const { return _value; }

private:
    vml::float4 _value;
    int numc;
};

class TextureConstant: public Constant {
public:
    explicit TextureConstant(std::string name, TextureType const* type,
                             AssetHandle asset);

    DECLARE_TYPE(TextureType)

    AssetHandle asset() const { return _asset; }

    int bindingPoint() const { return _bindingPoint; }

    void setBindingPoint(int bindingPoint) { _bindingPoint = bindingPoint; }

private:
    AssetHandle _asset;
    int _bindingPoint = -1;
};

class RasterizerInput: public Constant {
public:
    DECLARE_TYPE(VectorType)

    explicit RasterizerInput(Context& ctx, std::string name,
                             RasterizerInputKind kind);

    RasterizerInputKind kind() const { return _kind; }

private:
    RasterizerInputKind _kind;
};

class Instruction: public User, public utl::ilist_node<Instruction> {
protected:
    using User::User;
};

class Callable: public Constant {
public:
    DECLARE_TYPE(FunctionType)

    Type const* returnType() const;

    List<FunctionParameter> const& parameters() const { return _parameters; }

protected:
    Callable(ValueNodeType nodeType, std::string name, FunctionType const* type,
             List<FunctionParameter> parameters);

private:
    List<FunctionParameter> _parameters;
};

class Function: public Callable {
public:
    explicit Function(std::string name, FunctionType const* type,
                      List<FunctionParameter> parameters,
                      List<Instruction> body = {});

    /// Container interface @{
    auto begin() { return _body.begin(); }
    auto begin() const { return _body.begin(); }
    auto end() { return _body.end(); }
    auto end() const { return _body.end(); }
    bool empty() const { return _body.empty(); }
    /// @}

private:
    List<Instruction> _body;
};

class BuiltinFunction: public Callable {
public:
    explicit BuiltinFunction(Context& ctx, BuiltinFunctionKind kind,
                             std::span<Type const* const> argTypes);

    BuiltinFunctionKind kind() const { return _kind; }

private:
    BuiltinFunctionKind _kind;
};

class FunctionCall: public Instruction {
public:
    FunctionCall(Context& ctx, std::string name, BuiltinFunctionKind function,
                 std::span<Value* const> arguments);

    FunctionCall(Context& ctx, std::string name, BuiltinFunctionKind function,
                 std::span<Type const* const> argumentTypes);

    DECLARE_OPERAND(BuiltinFunction, function, Function, 0)

    DECLARE_OPERAND_RANGE(Value, argument, Argument, 1)

private:
    struct PrivArgs;
    FunctionCall(PrivArgs args);
};

class ArithmeticOperation: public Instruction {
public:
    DECLARE_TYPE(VectorType)

    enum Kind { Add, Sub, Mul, Div };

    static Type const* commonType(Type const* lhs, Type const* rhs);

    ArithmeticOperation(std::string name, Kind kind, Value* lhs, Value* rhs):
        Instruction(ValueNodeType::ArithmeticOperation, std::move(name),
                    commonType(lhs->type(), rhs->type()), { lhs, rhs }),
        _kind(kind) {}

    ArithmeticOperation(Kind kind, std::string name, Type const* lhsType,
                        Type const* rhsType):
        Instruction(ValueNodeType::ArithmeticOperation, std::move(name),
                    commonType(lhsType, rhsType), { nullptr, nullptr }),
        _kind(kind) {}

    DECLARE_OPERAND(Value, LHS, LHS, 0)
    DECLARE_OPERAND(Value, RHS, RHS, 1)

    Kind kind() const { return _kind; }

private:
    Kind _kind;
};

class TextureSampler: public Instruction {
public:
    DECLARE_TYPE(VectorType)

    TextureSampler(Context& ctx, std::string name, TextureConstant* texture,
                   Value* coords, SampleMode mode);

    DECLARE_OPERAND(TextureConstant, texture, Texture, 0)
    DECLARE_OPERAND(Value, coords, Coords, 1)

    SampleMode mode() const { return _mode; }

private:
    SampleMode _mode;
};

class VectorBuilder: public Instruction {
public:
    DECLARE_TYPE(VectorType)

    explicit VectorBuilder(Context& ctx, std::string name,
                           std::vector<Value*> values);

    explicit VectorBuilder(std::string name, VectorType const* type,
                           int numInputs);
};

class ExtractVectorValue: public Instruction {
public:
    DECLARE_TYPE(VectorType)

    explicit ExtractVectorValue(Context& ctx, std::string name, int index,
                                Value* operand);

    int index() const { return _index; }

    DECLARE_OPERAND(Value, base, Base, 0)

private:
    int _index = -1;
};

class AggregateBuilder: public Instruction {
public:
    DECLARE_TYPE(AggregateType)

    explicit AggregateBuilder(std::string name, AggregateType const* type,
                              std::vector<Value*> values);

    explicit AggregateBuilder(std::string name, AggregateType const* type,
                              std::initializer_list<Value*> values):
        AggregateBuilder(std::move(name), type, std::vector(values)) {}

    explicit AggregateBuilder(std::string name, AggregateType const* type);
};

/// Element access into an aggregate, i.e., `value.member`
class ExtractAggregateValue: public Instruction {
public:
    /// \p baseValue may not be null and must be of type `AggregateType`
    explicit ExtractAggregateValue(std::string name, Value* baseValue,
                                   int index);

    /// \p elementType is the type of the extracted element
    explicit ExtractAggregateValue(std::string name, Type const* elementType,
                                   int index);

    int index() const { return _index; }

    DECLARE_OPERAND(Value, base, Base, 0)

private:
    int _index;
};

} // namespace bloom::matc

#undef DECLARE_TYPE
#undef DECLARE_OPERAND
#undef DECLARE_OPERAND_RANGE

#endif // BLOOM_MATERIALCOMPILER_VALUES_H

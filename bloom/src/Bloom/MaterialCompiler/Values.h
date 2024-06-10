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
#include "Bloom/MaterialCompiler/Types.h"

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
    template <typename T = TYPE>                                               \
    T* NAME(size_t index) const {                                              \
        return NAME##s()[index];                                               \
    }                                                                          \
    void set##CAP_NAME(size_t index, Value* value) {                           \
        User::setOperand(index + INDEX_OFFSET, value);                         \
    }

namespace bloom::matc {

template <typename T>
class DynAllocator: public std::allocator<T> {
public:
    void destroy(T* ptr) { utl::dyn_destroy(ptr); }

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

    void setName(std::string name) { _name = std::move(name); }

    auto users() const { return _users.values() | ranges::views::keys; }

    void detachAllUsers();

    void replace(Value* other);

protected:
    Value(ValueNodeType nodeType, std::string name, Type const* type):
        _nodeType(nodeType), _name(std::move(name)), _type(type) {}
    Value(Value const&) = default;

private:
    friend ValueNodeType dyncast_get_type(Value const& value) {
        return value._nodeType;
    }

    friend class User; // To add and remove users
    void addUser(User* user);
    void removeUser(User* user);

    friend class Callable; // To set the type after construction

    ValueNodeType _nodeType;
    std::string _name;
    Type const* _type;
    utl::hashmap<User*, int> _users;
};

struct ParameterDescriptor {
    std::string name = {};
    Type const* type = nullptr;
    ParameterQualifier qualifier = {};
    ParameterAttribute attribute = {};
};

class FunctionParameter: public Value, public ListNode<FunctionParameter> {
public:
    explicit FunctionParameter(ParameterDescriptor desc):
        Value(ValueNodeType::FunctionParameter, std::move(desc.name),
              desc.type),
        _qual(desc.qualifier),
        _attrib(desc.attribute) {}

    ParameterQualifier qualifier() const { return _qual; }

    ParameterAttribute attribute() const { return _attrib; }

private:
    ParameterQualifier _qual;
    ParameterAttribute _attrib;
};

class User: public Value {
public:
    ~User();

    std::span<Value* const> operands() const { return _operands; }

    size_t numOperands() const { return operands().size(); }

    Value* operand(size_t index) const {
        assert(index < _operands.size());
        return _operands[index];
    }

    void setOperand(size_t index, Value* value);

    void replaceOperand(Value* existing, Value* replacement);

protected:
    User(ValueNodeType nodeType, std::string name, Type const* type,
         std::vector<Value*> operands = {});

    User(User const&);

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

class IntConstant: public Constant {
public:
    explicit IntConstant(std::string name, IntType const* type,
                         long long value);

    DECLARE_TYPE(IntType)

    long long get() const { return _value; }

private:
    long long _value;
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

class Instruction: public User, public ListNode<Instruction> {
public:
    DynUniquePtr<Instruction> clone() const;

    Function* function() const { return _function; }

protected:
    using User::User;

private:
    DynUniquePtr<Instruction> doClone() const;
    friend class Function;
    Function* _function = nullptr;
};

class Callable: public Constant {
public:
    DECLARE_TYPE(FunctionType)

    Type const* returnType() const;

    List<FunctionParameter>& parameters() { return _parameters; }
    List<FunctionParameter> const& parameters() const { return _parameters; }

    void setParameters(List<FunctionParameter> params);

    FunctionParameter* addParameter(FunctionParameter* param) {
        _parameters.push_back(param);
        return param;
    }

    FunctionParameter* addParameter(ParameterDescriptor desc) {
        return addParameter(new FunctionParameter(desc));
    }

    FunctionType const* computeType(Context& ctx, Type const* returnType);

protected:
    Callable(ValueNodeType nodeType, std::string name, FunctionType const* type,
             List<FunctionParameter> parameters);

private:
    List<FunctionParameter> _parameters;
};

List<FunctionParameter> allocateParameters(
    std::span<ParameterDescriptor const> args);

class Function: public Callable {
public:
    explicit Function(std::string name, FunctionType const* type,
                      ShaderKind shaderKind = ShaderKind::None,
                      List<FunctionParameter> parameters = {},
                      List<Instruction> body = {});

    /// Container interface @{
    using iterator = List<Instruction>::iterator;
    using const_iterator = List<Instruction>::const_iterator;
    Instruction* push_front(DynUniquePtr<Instruction> inst) {
        return push_front(inst.release());
    }
    Instruction* push_front(Instruction* inst);
    Instruction* push_back(DynUniquePtr<Instruction> inst) {
        return push_back(inst.release());
    }
    Instruction* push_back(Instruction* inst);
    Instruction* insert(const_iterator before, Instruction* inst) {
        _body.insert(before, inst);
        return inst;
    }
    iterator begin() { return _body.begin(); }
    const_iterator begin() const { return _body.begin(); }
    iterator end() { return _body.end(); }
    const_iterator end() const { return _body.end(); }
    bool empty() const { return _body.empty(); }
    Instruction& back() { return _body.back(); }
    Instruction const& back() const { return _body.back(); }
    Instruction& front() { return _body.front(); }
    Instruction const& front() const { return _body.front(); }
    /// @}

    ShaderKind shaderKind() const { return _shaderKind; }

    ReturnInst* getReturn() {
        return const_cast<ReturnInst*>(std::as_const(*this).getReturn());
    }

    ReturnInst const* getReturn() const;

private:
    List<Instruction> _body;
    ShaderKind _shaderKind;
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
    FunctionCall(std::string name, Callable* function,
                 std::span<Value* const> arguments);

    FunctionCall(Context& ctx, std::string name, BuiltinFunctionKind function,
                 std::span<Value* const> arguments);

    FunctionCall(Context& ctx, std::string name, BuiltinFunctionKind function,
                 std::span<Type const* const> argumentTypes);

    DECLARE_OPERAND(Callable, function, Function, 0)

    DECLARE_OPERAND_RANGE(Value, argument, Argument, 1)

private:
    struct PrivArgs;
    FunctionCall(PrivArgs args);
};

class ArithmeticInst: public Instruction {
public:
    DECLARE_TYPE(VectorType)

    enum Operation { Add, Sub, Mul, Div };

    ArithmeticInst(Context& ctx, std::string name, Operation operation,
                   Value* lhs, Value* rhs):
        Instruction(ValueNodeType::ArithmeticInst, std::move(name),
                    commonType(ctx, lhs->type(), rhs->type()), { lhs, rhs }),
        _operation(operation) {}

    ArithmeticInst(Context& ctx, std::string name, Operation operation,
                   Type const* lhsType, Type const* rhsType):
        Instruction(ValueNodeType::ArithmeticInst, std::move(name),
                    commonType(ctx, lhsType, rhsType), { nullptr, nullptr }),
        _operation(operation) {}

    DECLARE_OPERAND(Value, LHS, LHS, 0)
    DECLARE_OPERAND(Value, RHS, RHS, 1)

    Operation operation() const { return _operation; }

private:
    static Type const* commonType(Context& ctx, Type const* lhs,
                                  Type const* rhs);

    Operation _operation;
};

class CompareInst: public Instruction {
public:
    DECLARE_TYPE(BoolType)

    enum Operation { Less, LessEq, Greater, GreaterEq, Equal, NotEqual };

    CompareInst(Context& ctx, std::string name, Operation operation, Value* lhs,
                Value* rhs);

    DECLARE_OPERAND(Value, LHS, LHS, 0)
    DECLARE_OPERAND(Value, RHS, RHS, 1)

    Operation operation() const { return _operation; }

private:
    Operation _operation;
};

class TextureSampler: public Instruction {
public:
    DECLARE_TYPE(VectorType)

    TextureSampler(Context& ctx, std::string name, Value* texture,
                   Value* coords, SampleMode mode);

    DECLARE_OPERAND(Value, texture, Texture, 0)
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

    explicit ExtractVectorValue(Context& ctx, std::string name, Value* operand,
                                vml::int4 indices, int numComponents);

    explicit ExtractVectorValue(Context& ctx, std::string name, Value* operand,
                                int indexA, int indexB = -1, int indexC = -1,
                                int indexD = -1);

    vml::int4 indices() const { return _indices; }

    int numComponents() const { return _numComponents; }

    DECLARE_OPERAND(Value, base, Base, 0)

private:
    vml::int4 _indices{ -1 };
    int _numComponents;
};

class AggregateBuilder: public Instruction {
public:
    DECLARE_TYPE(AggregateType)

    DECLARE_OPERAND_RANGE(Value, element, Element, 0)

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

class ArrayAccess: public Instruction {
public:
    explicit ArrayAccess(std::string name, Value* array, Value* index);

    DECLARE_OPERAND(Value, array, Array, 0)

    DECLARE_OPERAND(Value, index, Index, 1)
};

class DiscardFragment: public Instruction {
public:
    explicit DiscardFragment(Context& ctx, std::string name, Value* condition);

    DECLARE_OPERAND(Value, condition, Condition, 0)
};

class ReturnInst: public Instruction {
public:
    explicit ReturnInst(Context& ctx, std::string name, Value* retVal);

    DECLARE_OPERAND(Value, returnValue, ReturnValue, 0)
};

} // namespace bloom::matc

#undef DECLARE_TYPE
#undef DECLARE_OPERAND
#undef DECLARE_OPERAND_RANGE

#endif // BLOOM_MATERIALCOMPILER_VALUES_H

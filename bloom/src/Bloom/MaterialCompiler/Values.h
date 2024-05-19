#ifndef BLOOM_MATERIALCOMPILER_VALUES_H
#define BLOOM_MATERIALCOMPILER_VALUES_H

#include <span>
#include <string>

#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/MaterialCompiler/Fwd.h"

#define DECLARE_TYPE(TYPE)                                                     \
    template <typename T = TYPE>                                               \
    T const* type() const {                                                    \
        return cast<T const*>(Value::type());                                  \
    }

namespace bloom::matc {

class Value {
public:
    Type const* type() const { return _type; }

protected:
    Value(ValueNodeType nodeType, Type const* type):
        _nodeType(nodeType), _type(type) {}

private:
    friend ValueNodeType dyncast_get_type(Value const& value) {
        return value._nodeType;
    }

    ValueNodeType _nodeType;
    Type const* _type;
};

class VectorValue: public Value {
public:
    DECLARE_TYPE(VectorType)

    int numComponents() const { return numc; }

    vml::float4 get() const { return _value; }

protected:
    explicit VectorValue(ValueNodeType nodeType, Context& ctx,
                         int numComponents, vml::float4 value);

private:
    vml::float4 _value;
    int numc;
};

class VectorConstant: public VectorValue {
public:
    explicit VectorConstant(Context& ctx, int numComponents, vml::float4 value):
        VectorValue(ValueNodeType::VectorConstant, ctx, numComponents, value) {}
};

class VectorParameter: public VectorValue {
public:
    explicit VectorParameter(Context& ctx, int numComponents,
                             vml::float4 defaultValue, std::string name):
        VectorValue(ValueNodeType::VectorParameter, ctx, numComponents,
                    defaultValue),
        _name(std::move(name)) {}

    std::string const& name() const { return _name; }

private:
    std::string _name;
};

class TextureValue: public Value {
public:
    DECLARE_TYPE(TextureType)

    AssetHandle asset() const { return _asset; }

protected:
    explicit TextureValue(ValueNodeType nodeType, TextureType const* type,
                          AssetHandle asset);

private:
    AssetHandle _asset;
};

class TextureConstant: public TextureValue {
public:
    explicit TextureConstant(TextureType const* type, AssetHandle asset):
        TextureValue(ValueNodeType::TextureConstant, type, asset) {}
};

class TextureParameter: public TextureValue {
public:
    explicit TextureParameter(TextureType const* type, AssetHandle asset,
                              std::string name):
        TextureValue(ValueNodeType::TextureValue, type, asset),
        _name(std::move(name)) {}

    std::string const& name() const { return _name; }

private:
    std::string _name;
};

class RasterizerInput: public Value {
public:
    DECLARE_TYPE(VectorType)

    explicit RasterizerInput(Context& ctx, RasterizerInputKind kind);

    RasterizerInputKind kind() const { return _kind; }

private:
    RasterizerInputKind _kind;
};

class User: public Value {
public:
    std::span<Value* const> operands() const { return _operands; }

    size_t numOperands() const { return operands().size(); }

    Value* operand(size_t index) const {
        assert(index < _operands.size());
        return _operands[index];
    }

    void setOperand(size_t index, Value* value) {
        assert(index < _operands.size());
        _operands[index] = value;
    }

protected:
    User(ValueNodeType nodeType, Type const* type,
         std::vector<Value*> operands):
        Value(nodeType, type), _operands(std::move(operands)) {}

private:
    std::vector<Value*> _operands;
};

class BuiltinFunction: public User {
public:
    BuiltinFunction(Context& ctx, BuiltinFunctionKind kind,
                    std::vector<Value*> arguments);

    BuiltinFunction(Context& ctx, BuiltinFunctionKind kind,
                    std::span<Type const* const> argumentTypes);

    BuiltinFunctionKind kind() const { return _kind; }

private:
    BuiltinFunctionKind _kind;
};

class ArithmeticOperation: public User {
public:
    DECLARE_TYPE(VectorType)

    enum Kind { Add, Sub, Mul, Div };

    static Type const* commonType(Type const* lhs, Type const* rhs);

    ArithmeticOperation(Kind kind, Value* lhs, Value* rhs):
        User(ValueNodeType::ArithmeticOperation,
             commonType(lhs->type(), rhs->type()), { lhs, rhs }),
        _kind(kind) {}

    ArithmeticOperation(Kind kind, Type const* lhsType, Type const* rhsType):
        User(ValueNodeType::ArithmeticOperation, commonType(lhsType, rhsType),
             { nullptr, nullptr }),
        _kind(kind) {}

    Value* LHS() const { return operand(0); }

    Value* RHS() const { return operand(1); }

    Kind kind() const { return _kind; }

private:
    Kind _kind;
};

class TextureSampler: public User {
public:
    DECLARE_TYPE(VectorType)

    TextureSampler(Context& ctx, TextureConstant* texture, Value* coords,
                   SampleMode mode);

    TextureConstant* texture() const;

    Value* coords() const { return operand(1); }

    SampleMode mode() const { return _mode; }

private:
    SampleMode _mode;
};

class VectorBuilder: public User {
public:
    DECLARE_TYPE(VectorType)

    explicit VectorBuilder(Context& ctx, std::vector<Value*> values);

    explicit VectorBuilder(VectorType const* type, int numInputs);
};

class VectorElement: public User {
public:
    DECLARE_TYPE(VectorType)

    explicit VectorElement(Context& ctx, int index, Value* operand);

    int index() const { return _index; }

    Value* operand() const { return User::operand(0); }

private:
    int _index = -1;
};

class AggregateBuilder: public User {
public:
    DECLARE_TYPE(AggregateType)

    explicit AggregateBuilder(Context& ctx, std::vector<Value*> values);

    explicit AggregateBuilder(AggregateType const* type);
};

/// Element access into an aggregate, i.e., `value.member`
class AggregateElement: public User {
public:
    /// \p baseValue may not be null and must be of type `AggregateType`
    explicit AggregateElement(Value* baseValue, int index);

    /// \p elementType is the type of the extracted element
    explicit AggregateElement(Type const* elementType, int index);

    int index() const { return _index; }

    Value* operand() const { return User::operand(0); }

private:
    int _index;
};

} // namespace bloom::matc

#undef DECLARE_TYPE

#endif // BLOOM_MATERIALCOMPILER_VALUES_H

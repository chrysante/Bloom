#ifndef BLOOM_MATERIALCOMPILER_VALUES_H
#define BLOOM_MATERIALCOMPILER_VALUES_H

#include <span>

#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/MaterialCompiler/Fwd.h"

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

class Constant: public Value {
public:
    using Value::Value;
};

class VectorConstant: public Constant {
public:
    explicit VectorConstant(Context& ctx, int numComponents, vml::float4 value);

    int numComponents() const { return numc; }

    vml::float4 get() const { return _value; }

private:
    vml::float4 _value;
    int numc;
};

class TextureConstant: public Constant {
public:
    explicit TextureConstant(TextureType const* type, AssetHandle asset);

    AssetHandle asset() const { return _asset; }

private:
    AssetHandle _asset;
};

class RasterizerInput: public Constant {
public:
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

    BuiltinFunction(Context& ctx, BuiltinFunctionKind kind);

    BuiltinFunctionKind kind() const { return _kind; }

private:
    BuiltinFunctionKind _kind;
};

class ArithmeticOperation: public User {
public:
    enum Kind { Add, Sub, Mul, Div };

    static Type const* commonType(Type const* lhs, Type const* rhs);

    ArithmeticOperation(Kind kind, Value* lhs, Value* rhs):
        User(ValueNodeType::ArithmeticOperation,
             commonType(lhs->type(), rhs->type()), { lhs, rhs }),
        _kind(kind) {}

    Value* LHS() const { return operand(0); }
    Value* RHS() const { return operand(1); }

    Kind kind() const { return _kind; }

private:
    Kind _kind;
};

class TextureSampler: public User {
public:
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
    explicit VectorBuilder(Context& ctx, std::vector<Value*> values);

    explicit VectorBuilder(VectorType const* type, int numInputs);
};

class VectorElement: public User {
public:
    explicit VectorElement(Context& ctx, int index, Value* operand);

    int index() const { return _index; }

    Value* operand() const { return User::operand(0); }

private:
    int _index = -1;
};

class AggregateBuilder: public User {
public:
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

#endif // BLOOM_MATERIALCOMPILER_VALUES_H

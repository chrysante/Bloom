#ifndef BLOOM_GRAPHICS_MATERIAL_VALUES_H
#define BLOOM_GRAPHICS_MATERIAL_VALUES_H

#include <span>

#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/MaterialCompiler/Fwd.h"

namespace bloom::matgraph {

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

class FloatConstant: public Constant {
public:
    explicit FloatConstant(Context& ctx, float value);

    float get() const { return _value; }

private:
    float _value;
};

class Float3Constant: public Constant {
public:
    Float3Constant(Context& ctx, vml::float3 value);

    vml::float3 get() const { return _value; }

private:
    vml::float3 _value;
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

class AggregateBuilder: public User {
public:
    explicit AggregateBuilder(Context& ctx, std::vector<Value*> values);

    explicit AggregateBuilder(AggregateType const* type);
};

} // namespace bloom::matgraph

#endif // BLOOM_GRAPHICS_MATERIAL_VALUES_H

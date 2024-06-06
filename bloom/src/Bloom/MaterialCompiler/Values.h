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

#define DECLARE_OPERAND(TYPE, NAME, INDEX)                                     \
    template <typename T = TYPE>                                               \
    T* NAME() const {                                                          \
        return cast<T*>(User::operand(INDEX));                                 \
    }

namespace bloom::matc {

class Value {
public:
    Type const* type() const { return _type; }

    std::string const& name() const { return _name; }

protected:
    Value(ValueNodeType nodeType, std::string name, Type const* type):
        _nodeType(nodeType), _name(std::move(name)), _type(type) {}

private:
    friend ValueNodeType dyncast_get_type(Value const& value) {
        return value._nodeType;
    }

    ValueNodeType _nodeType;
    std::string _name;
    Type const* _type;
};

class VectorValue: public Value {
public:
    DECLARE_TYPE(VectorType)

    int numComponents() const { return numc; }

    vml::float4 get() const { return _value; }

protected:
    explicit VectorValue(ValueNodeType nodeType, std::string name, Context& ctx,
                         int numComponents, vml::float4 value);

private:
    vml::float4 _value;
    int numc;
};

class VectorConstant: public VectorValue {
public:
    explicit VectorConstant(Context& ctx, std::string name, int numComponents,
                            vml::float4 value):
        VectorValue(ValueNodeType::VectorConstant, std::move(name), ctx,
                    numComponents, value) {}
};

class VectorParameter: public VectorValue {
public:
    explicit VectorParameter(Context& ctx, std::string name, int numComponents,
                             vml::float4 defaultValue,
                             BufferParameter* buffer = nullptr):
        VectorValue(ValueNodeType::VectorParameter, std::move(name), ctx,
                    numComponents, defaultValue),
        _buffer(buffer) {}

    BufferParameter* buffer() const { return _buffer; }

    int elementIndex() const { return _elemIndex; }

    void setBufferAndElemIndex(BufferParameter* buffer, int index) {
        _buffer = buffer;
        _elemIndex = index;
    }

private:
    BufferParameter* _buffer = nullptr;
    int _elemIndex = -1;
};

class TextureValue: public Value {
public:
    DECLARE_TYPE(TextureType)

    AssetHandle asset() const { return _asset; }

    int bindingPoint() const { return _bindingPoint; }

    void setBindingPoint(int bindingPoint) { _bindingPoint = bindingPoint; }

protected:
    explicit TextureValue(ValueNodeType nodeType, std::string name,
                          TextureType const* type, AssetHandle asset);

private:
    AssetHandle _asset;
    int _bindingPoint = -1;
};

class TextureConstant: public TextureValue {
public:
    explicit TextureConstant(std::string name, TextureType const* type,
                             AssetHandle asset):
        TextureValue(ValueNodeType::TextureConstant, std::move(name), type,
                     asset) {}
};

class TextureParameter: public TextureValue {
public:
    explicit TextureParameter(std::string name, TextureType const* type,
                              AssetHandle asset):
        TextureValue(ValueNodeType::TextureParameter, std::move(name), type,
                     asset) {}
};

class BufferParameter: public Value {
public:
    explicit BufferParameter(std::string name, Type const* type,
                             int bindingPoint):
        Value(ValueNodeType::BufferParameter, std::move(name), type),
        _bindingPoint(bindingPoint) {}

    int bindingPoint() const { return _bindingPoint; }

    void setBindingPoint(int bindingPoint) { _bindingPoint = bindingPoint; }

private:
    int _bindingPoint;
};

class RasterizerInput: public Value {
public:
    DECLARE_TYPE(VectorType)

    explicit RasterizerInput(Context& ctx, std::string name,
                             RasterizerInputKind kind);

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
    User(ValueNodeType nodeType, std::string name, Type const* type,
         std::vector<Value*> operands):
        Value(nodeType, std::move(name), type),
        _operands(std::move(operands)) {}

private:
    std::vector<Value*> _operands;
};

class FunctionCall: public User {
public:
    FunctionCall(Context& ctx, std::string name, BuiltinFunction function,
                 std::vector<Value*> arguments);

    FunctionCall(Context& ctx, std::string name, BuiltinFunction function,
                 std::span<Type const* const> argumentTypes);

    BuiltinFunction function() const { return _function; }

private:
    BuiltinFunction _function;
};

class ArithmeticOperation: public User {
public:
    DECLARE_TYPE(VectorType)

    enum Kind { Add, Sub, Mul, Div };

    static Type const* commonType(Type const* lhs, Type const* rhs);

    ArithmeticOperation(std::string name, Kind kind, Value* lhs, Value* rhs):
        User(ValueNodeType::ArithmeticOperation, std::move(name),
             commonType(lhs->type(), rhs->type()), { lhs, rhs }),
        _kind(kind) {}

    ArithmeticOperation(Kind kind, std::string name, Type const* lhsType,
                        Type const* rhsType):
        User(ValueNodeType::ArithmeticOperation, std::move(name),
             commonType(lhsType, rhsType), { nullptr, nullptr }),
        _kind(kind) {}

    DECLARE_OPERAND(Value, LHS, 0)
    DECLARE_OPERAND(Value, RHS, 1)

    Kind kind() const { return _kind; }

private:
    Kind _kind;
};

class TextureSampler: public User {
public:
    DECLARE_TYPE(VectorType)

    TextureSampler(Context& ctx, std::string name, TextureValue* texture,
                   Value* coords, SampleMode mode);

    DECLARE_OPERAND(TextureValue, texture, 0)
    DECLARE_OPERAND(Value, coords, 1)

    SampleMode mode() const { return _mode; }

private:
    SampleMode _mode;
};

class VectorBuilder: public User {
public:
    DECLARE_TYPE(VectorType)

    explicit VectorBuilder(Context& ctx, std::string name,
                           std::vector<Value*> values);

    explicit VectorBuilder(std::string name, VectorType const* type,
                           int numInputs);
};

class ExtractVectorValue: public User {
public:
    DECLARE_TYPE(VectorType)

    explicit ExtractVectorValue(Context& ctx, std::string name, int index,
                                Value* operand);

    int index() const { return _index; }

    DECLARE_OPERAND(Value, operand, 0)

private:
    int _index = -1;
};

class AggregateBuilder: public User {
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
class ExtractAggregateValue: public User {
public:
    /// \p baseValue may not be null and must be of type `AggregateType`
    explicit ExtractAggregateValue(std::string name, Value* baseValue,
                                   int index);

    /// \p elementType is the type of the extracted element
    explicit ExtractAggregateValue(std::string name, Type const* elementType,
                                   int index);

    int index() const { return _index; }

    DECLARE_OPERAND(Value, operand, 0)

private:
    int _index;
};

} // namespace bloom::matc

#undef DECLARE_TYPE
#undef DECLARE_OPERAND

#endif // BLOOM_MATERIALCOMPILER_VALUES_H

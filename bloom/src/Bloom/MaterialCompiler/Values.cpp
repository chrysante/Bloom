#include "Bloom/MaterialCompiler//Values.h"

#include <range/v3/numeric.hpp>
#include <range/v3/view.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace vml::short_types;
using namespace ranges::views;

VectorValue::VectorValue(ValueNodeType nodeType, Context& ctx,
                         int numComponents, float4 value):
    Value(nodeType, ctx.vectorType(numComponents)),
    _value(value),
    numc(numComponents) {}

TextureValue::TextureValue(ValueNodeType nodeType, TextureType const* type,
                           AssetHandle asset):
    Value(nodeType, type), _asset(asset) {}

static Type const* selectType(Context& ctx, RasterizerInputKind kind) {
    using enum RasterizerInputKind;
    switch (kind) {
    case TextureCoords:
        return ctx.vectorType(2);
    case VertexNormals:
        [[fallthrough]];
    case VertexTangents:
        [[fallthrough]];
    case VertexBinormals:
        [[fallthrough]];
    case WorldPosition:
        return ctx.vectorType(3);
    }
}

RasterizerInput::RasterizerInput(Context& ctx, RasterizerInputKind kind):
    Value(ValueNodeType::RasterizerInput, selectType(ctx, kind)), _kind(kind) {}

static Type const* getReturnType(Context&, BuiltinFunctionKind kind,
                                 std::span<Type const* const> argTypes) {
    using enum BuiltinFunctionKind;
    switch (kind) {
    case Normalize:
        [[fallthrough]];
    case Sqrt:
        assert(argTypes.size() == 1);
        return argTypes[0];
    }
}

BuiltinFunction::BuiltinFunction(Context& ctx, BuiltinFunctionKind kind,
                                 std::vector<Value*> arguments):
    User(ValueNodeType::BuiltinFunction,
         getReturnType(ctx, kind,
                       arguments | transform(&Value::type) | ToSmallVector<>),
         std::move(arguments)),
    _kind(kind) {}

BuiltinFunction::BuiltinFunction(Context& ctx, BuiltinFunctionKind kind,
                                 std::span<Type const* const> argumentTypes):
    User(ValueNodeType::BuiltinFunction,
         getReturnType(ctx, kind, argumentTypes),
         std::vector<Value*>(argumentTypes.size())),
    _kind(kind) {}

static void throwArithmeticTypeError() {
    throw std::runtime_error("Invalid operand types for arithmetic operation");
}

Type const* ArithmeticOperation::commonType(Type const* lhs, Type const* rhs) {
    auto* LHS = dyncast<VectorType const*>(lhs);
    auto* RHS = dyncast<VectorType const*>(rhs);
    if (!LHS || !RHS) {
        throwArithmeticTypeError();
    }
    if (LHS->numComponents() == 1) {
        return RHS;
    }
    if (RHS->numComponents() == 1) {
        return LHS;
    }
    if (LHS->numComponents() != RHS->numComponents()) {
        throwArithmeticTypeError();
    }
    return LHS;
}

TextureSampler::TextureSampler(Context& ctx, TextureConstant* texture,
                               Value* coords, SampleMode mode):
    User(ValueNodeType::TextureSampler, ctx.float4Type(), { texture, coords }),
    _mode(mode) {}

TextureConstant* TextureSampler::texture() const {
    return cast<TextureConstant*>(operand(0));
}

static int accNumComponents(std::span<Value const* const> values) {
    return ranges::accumulate(values, 0, ranges::plus{},
                              [](Value const* value) {
        return cast<VectorType const*>(value->type())->numComponents();
    });
}

VectorBuilder::VectorBuilder(Context& ctx, std::vector<Value*> values):
    User(ValueNodeType::VectorBuilder, ctx.vectorType(accNumComponents(values)),
         std::move(values)) {}

VectorBuilder::VectorBuilder(VectorType const* type, int numInputs):
    User(ValueNodeType::VectorBuilder, type, std::vector<Value*>(numInputs)) {}

VectorElement::VectorElement(Context& ctx, int index, Value* operand):
    User(ValueNodeType::VectorElement, ctx.floatType(), { operand }),
    _index(index) {}

AggregateBuilder::AggregateBuilder(Context& ctx, std::vector<Value*> values):
    User(ValueNodeType::AggregateBuilder,
         ctx.aggregateType(values | ranges::views::transform(&Value::type) |
                           ToSmallVector<>),
         std::move(values)) {}

AggregateBuilder::AggregateBuilder(AggregateType const* type):
    User(ValueNodeType::AggregateBuilder, type,
         std::vector<Value*>(type->elements().size())) {}

AggregateElement::AggregateElement(Value* baseValue, int index):
    User(ValueNodeType::AggregateElement,
         cast<AggregateType const*>(baseValue->type())->element(index),
         { baseValue }),
    _index(index) {}

AggregateElement::AggregateElement(Type const* elementType, int index):
    User(ValueNodeType::AggregateElement, elementType, std::vector<Value*>(1)),
    _index(index) {}

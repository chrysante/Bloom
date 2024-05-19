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

namespace {

enum class SymType {
    Float,  /// Single floating point value
    Float3, /// Vector of three floating point values
    FloatN, /// Single floating point value or vector of 2-4 floating point
            /// values
    VectorN /// Vector of 2-4 floating point values
};

} // namespace

static Type const* deduceRetType(Context& ctx, SymType symRet,
                                 std::span<SymType const> symArgs,
                                 std::span<Type const* const> args) {
    using enum SymType;
    if (symArgs.size() != args.size()) {
        return nullptr;
    }
    int N = -1;
    for (auto [symArg, arg]: zip(symArgs, args)) {
        switch (symArg) {
        case Float:
            if (arg != ctx.floatType()) {
                return nullptr;
            }
            break;
        case Float3:
            if (arg != ctx.float3Type()) {
                return nullptr;
            }
            break;
        case FloatN: {
            auto* vecType = dyncast<VectorType const*>(arg);
            if (!vecType) {
                return nullptr;
            }
            if (N != -1 && N != vecType->numComponents()) {
                return nullptr;
            }
            N = vecType->numComponents();
            break;
        }
        case VectorN: {
            auto* vecType = dyncast<VectorType const*>(arg);
            if (!vecType || vecType->numComponents() < 2) {
                return nullptr;
            }
            if (N != -1 && N != vecType->numComponents()) {
                return nullptr;
            }
            N = vecType->numComponents();
            break;
        }
        }
    }
    switch (symRet) {
    case Float:
        return ctx.floatType();
    case Float3:
        return ctx.float3Type();
    case FloatN:
        [[fallthrough]];
    case VectorN:
        if (N == -1) {
            return nullptr;
        }
        return ctx.vectorType(N);
    }
}

static Type const* getReturnTypeImpl(Context& ctx, BuiltinFunctionKind kind,
                                     std::span<Type const* const> args) {
    switch (kind) {
        using enum SymType;
#define ARGS_TO_SPAN(...) std::span({ __VA_ARGS__ })
#define BL_MATGRAPH_BUILTIN(Name, Spelling, RetType, Args)                     \
    case BuiltinFunctionKind::Name:                                            \
        return deduceRetType(ctx, RetType, ARGS_TO_SPAN Args, args);
#include "Bloom/MaterialCompiler/Lists.def"
    }
}

static Type const* getReturnType(Context& ctx, BuiltinFunctionKind kind,
                                 std::span<Type const* const> args) {
    auto* type = getReturnTypeImpl(ctx, kind, args);
    if (!type) {
        throw std::runtime_error("Invalid arguments for function call");
    }
    return type;
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

TextureSampler::TextureSampler(Context& ctx, TextureValue* texture,
                               Value* coords, SampleMode mode):
    User(ValueNodeType::TextureSampler, ctx.float4Type(), { texture, coords }),
    _mode(mode) {}

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

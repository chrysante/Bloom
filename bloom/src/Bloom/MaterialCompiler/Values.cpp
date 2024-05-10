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

VectorConstant::VectorConstant(Context& ctx, int numComponents, float4 value):
    Constant(ValueNodeType::VectorConstant, ctx.vectorType(numComponents)),
    _value(value),
    numc(numComponents) {}

TextureConstant::TextureConstant(Context& ctx, AssetHandle asset):
    Constant(ValueNodeType::TextureConstant, ctx.textureType()),
    _asset(asset) {}

TextureCoords::TextureCoords(Context& ctx):
    Constant(ValueNodeType::TextureCoords, ctx.vectorType(2)) {}

Type const* ArithmeticOperation::commonType(Type const* lhs, Type const* rhs) {
    assert(lhs == rhs);
    return lhs;
}

TextureSampler::TextureSampler(Context& ctx, TextureConstant* texture,
                               Value* coords):
    User(ValueNodeType::TextureSampler, ctx.float4Type(), { texture, coords }) {
}

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

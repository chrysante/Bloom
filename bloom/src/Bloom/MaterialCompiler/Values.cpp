#include "Bloom/MaterialCompiler//Values.h"

#include <range/v3/view.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"

using namespace bloom;
using namespace matc;
using namespace vml::short_types;
using namespace ranges::views;

VectorConstant::VectorConstant(Context& ctx, int numComponents, float4 value):
    Constant(ValueNodeType::VectorConstant, ctx.vectorType(numComponents)),
    _value(value),
    numc(numComponents) {}

Type const* ArithmeticOperation::commonType(Type const* lhs, Type const* rhs) {
    assert(lhs == rhs);
    return lhs;
}

AggregateBuilder::AggregateBuilder(Context& ctx, std::vector<Value*> values):
    User(ValueNodeType::AggregateBuilder,
         ctx.aggregateType(values | ranges::views::transform(&Value::type) |
                           ToSmallVector<>),
         std::move(values)) {}

AggregateBuilder::AggregateBuilder(AggregateType const* type):
    User(ValueNodeType::AggregateBuilder, type,
         std::vector<Value*>(type->elements().size())) {}

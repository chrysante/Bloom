#include "Bloom/Graphics/Material/Values.h"

#include <range/v3/view.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/Material/Context.h"
#include "Bloom/Graphics/Material/Types.h"

using namespace bloom;
using namespace matgraph;
using namespace ranges::views;

FloatConstant::FloatConstant(Context& ctx, float value):
    Constant(ValueNodeType::FloatConstant, ctx.floatType()), _value(value) {}

Float3Constant::Float3Constant(Context& ctx, vml::float3 value):
    Constant(ValueNodeType::Float3Constant, ctx.float3Type()), _value(value) {}

Type const* ArithmeticOperation::commonType(Type const* lhs, Type const* rhs) {
    BL_UNIMPLEMENTED();
}

AggregateBuilder::AggregateBuilder(Context& ctx, std::vector<Value*> values):
    User(ValueNodeType::AggregateBuilder,
         ctx.aggregateType(values | ranges::views::transform(&Value::type) |
                           ToSmallVector<>),
         std::move(values)) {}

AggregateBuilder::AggregateBuilder(AggregateType const* type):
    User(ValueNodeType::AggregateBuilder, type,
         std::vector<Value*>(type->elements().size())) {}

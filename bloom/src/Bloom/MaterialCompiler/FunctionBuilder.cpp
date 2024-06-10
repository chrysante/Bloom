#include "Bloom/MaterialCompiler/FunctionBuilder.h"

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/overload.hpp>

using namespace bloom;
using namespace matc;
using namespace ranges::views;

static void checkArguments(std::span<Value const* const> args,
                           Callable const* callee) {
    assert(callee->type()->argumentTypes().size() == args.size());
    for (auto [a, b]:
         zip(callee->type()->argumentTypes(), args | transform(&Value::type)))
    {
        assert(a == b);
    }
}

FunctionCall* FunctionBuilder::callFunction(std::string name, Callable* callee,
                                            std::span<Value* const> args) {
    checkArguments(args, callee);
    return insert<FunctionCall>(std::move(name), callee, args);
}

Value* FunctionBuilder::getElem(std::string name, Value* base, int index) {
    // clang-format off
    return visit<Value*>(*base->type(), utl::overload {
        [&](VectorType const&) {
            return insert<ExtractVectorValue>(std::move(name), base, index);
        },
        [&](AggregateType const&) {
            return insert<ExtractAggregateValue>(std::move(name), base, index);
        },
        [&](Type const& ) {
            assert(false);
        },
    }); // clang-format on
}

VectorBuilder* FunctionBuilder::appendVector(std::string name,
                                             std::vector<Value*> values) {
    return insert<VectorBuilder>(std::move(name), std::move(values));
}

MatrixBuilder* FunctionBuilder::buildMatrix(std::string name,
                                            std::vector<Value*> columns) {
    return insert<MatrixBuilder>(std::move(name), std::move(columns));
}

AggregateBuilder* FunctionBuilder::makeAggregate(std::string name,
                                                 AggregateType const* type,
                                                 std::vector<Value*> values) {
    return insert<AggregateBuilder>(std::move(name), type, std::move(values));
}

Value* FunctionBuilder::clampMin(std::string name, Value* a, Value* b) {
    return callBuiltin(std::move(name), BuiltinFunctionKind::Min, { a, b });
}

Value* FunctionBuilder::clampMin(std::string name, Value* a, vml::float4 b) {
    auto* type = cast<VectorType const*>(a->type());
    return clampMin(std::move(name), a,
                    ctx->vectorConstant(type->numComponents(), b));
}

Value* FunctionBuilder::callBuiltin(std::string name,
                                    BuiltinFunctionKind function,
                                    std::span<Value* const> arguments) {
    return insert<FunctionCall>(std::move(name), function, arguments);
}

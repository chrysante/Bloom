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
        [&](VectorType const& type) {
            return insert<ExtractVectorValue>(std::move(name), base, index);
        },
        [&](AggregateType const& type) {
            return insert<ExtractAggregateValue>(std::move(name), base, index);
        },
        [&](Type const& ) {
            assert(false);
        },
    }); // clang-format on
}

Value* FunctionBuilder::appendVector(std::string name,
                                     std::vector<Value*> values) {
    return insert<VectorBuilder>(std::move(name), std::move(values));
}

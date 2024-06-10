#include "Bloom/MaterialCompiler/FunctionBuilder.h"

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

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

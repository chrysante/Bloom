#include "Bloom/MaterialCompiler/FunctionBuilder.h"

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

using namespace bloom;
using namespace matc;
using namespace ranges::views;

FunctionCall* FunctionBuilder::callFunction(std::string name, Callable* callee,
                                            std::span<Value* const> args) {
    assert(ranges::equal(callee->type()->argumentTypes(),
                         args | transform(&Value::type)));
    return insert<FunctionCall>(std::move(name), callee, args);
}

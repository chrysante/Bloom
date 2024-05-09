#include <cstdlib>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace bloom;
using namespace matc;

TEST_CASE() {
    Context ctx;
    MaterialGraph g;
    auto* c2 = ctx.vectorConstant(3, { 2, 1, 5, 0 });
    auto* c1 = ctx.vectorConstant(1, { 1, 0, 0, 0 });
    g.setRoot(allocate<AggregateBuilder>(ctx, std::vector<Value*>{ c1, c2 }));
    auto generator = makeCodeGenerator(ctx, g, {});
    std::cout << generator->run() << std::endl;
}

#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace bloom;
using namespace matc;

TEST_CASE() {
    Context ctx;
    MaterialGraph g;
    auto* c2 = g.addValue(allocate<FloatConstant>(ctx, 2));
    auto* c1 = g.addValue(allocate<FloatConstant>(ctx, 1));
    g.setRoot(allocate<AggregateBuilder>(ctx, std::vector<Value*>{ c1, c2 }));
    auto generator = makeCodeGenerator();
    std::cout << generator->run(g) << std::endl;
}

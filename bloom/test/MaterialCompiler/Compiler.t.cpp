#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace bloom;
using namespace matc;

TEST_CASE() {
    Context ctx;
    MaterialGraph g;
    auto* c2 =
        g.addValue(allocate<VectorConstant>(ctx, 3, vml::float4{ 2, 1, 5, 0 }));
    auto* c1 =
        g.addValue(allocate<VectorConstant>(ctx, 1, vml::float4{ 1, 0, 0, 0 }));
    g.setRoot(allocate<AggregateBuilder>(ctx, std::vector<Value*>{ c1, c2 }));
    auto generator = makeCodeGenerator(ctx, g);
    std::cout << generator->run() << std::endl;
}

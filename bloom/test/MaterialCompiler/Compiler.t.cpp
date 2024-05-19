#include <cstdlib>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace bloom;
using namespace matc;
using namespace vml::short_types;

static AssetHandle genTexture() {
    return AssetHandle(AssetType::Texture2D, utl::uuid::generate());
}

TEST_CASE() {
    Context ctx;
    MaterialGraph g(MaterialShadingModel::DefaultLit);
    auto* tex0 = ctx.textureConstant(TextureSubType::Texture2D, genTexture());
    auto* tex1 = ctx.textureConstant(TextureSubType::TextureCube, genTexture());
    auto* const0 =
        g.addValue<VectorParameter>(ctx, 1, float4{ 1, 0, 0, 0 }, "Param0");
    auto* const1 =
        g.addValue<VectorParameter>(ctx, 3, float4{ 2, 1, 5, 0 }, "Param1");
    auto* const2 =
        g.addValue<VectorParameter>(ctx, 2, float4{ 2, 1, 5, 0 }, "Param2");
    auto* vec0 =
        g.addValue<VectorBuilder>(ctx,
                                  std::vector<Value*>{
                                      ctx.vectorConstant(1, { 1, 0, 0, 0 }),
                                      ctx.vectorConstant(1, { 2, 0, 0, 0 }),
                                      ctx.vectorConstant(1, { 3, 0, 0, 0 }),
                                  });
    auto* coords = ctx.rasterizerInput(RasterizerInputKind::TextureCoords);
    auto* sample0 =
        g.addValue<TextureSampler>(ctx, tex0, coords, SampleMode::Normal);
    auto* aggregate =
        g.addValue<AggregateBuilder>(ctx,
                                     std::vector<Value*>{ const0, const1, vec0,
                                                          sample0, const2 });
    Value* resultVal = g.addValue<AggregateElement>(aggregate, 1);
    resultVal = g.addValue<BuiltinFunction>(ctx, BuiltinFunctionKind::Normalize,
                                            std::vector<Value*>{ resultVal });
    g.setRoot(resultVal);
    auto generator = makeCodeGenerator(ctx, g, {});
    auto result = generator->run();
    std::cout << result.source << std::endl;
}

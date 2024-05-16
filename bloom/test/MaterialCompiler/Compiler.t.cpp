#include <cstdlib>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"

using namespace bloom;
using namespace matc;

static AssetHandle genTexture() {
    return AssetHandle(AssetType::Texture2D, utl::uuid::generate());
}

TEST_CASE() {
    Context ctx;
    MaterialGraph g(MaterialShadingModel::Unlit);
    auto* tex0 = ctx.textureConstant(TextureSubType::Texture2D, genTexture());
    auto* tex1 = ctx.textureConstant(TextureSubType::TextureCube, genTexture());
    auto* const0 = ctx.vectorConstant(1, { 1, 0, 0, 0 });
    auto* const1 = ctx.vectorConstant(3, { 2, 1, 5, 0 });
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
        g.addValue<AggregateBuilder>(ctx, std::vector<Value*>{ const0, const1,
                                                               vec0, sample0 });
    Value* resultVal = g.addValue<AggregateElement>(aggregate, 1);
    resultVal = g.addValue<BuiltinFunction>(ctx, BuiltinFunctionKind::Normalize,
                                            std::vector<Value*>{ resultVal });
    auto* worldpos = ctx.rasterizerInput(RasterizerInputKind::WorldPosition);
    auto* col =
        g.addValue<TextureSampler>(ctx, tex1, worldpos, SampleMode::Color);
    g.setRoot(col);
    auto generator = makeCodeGenerator(ctx, g, {});
    auto result = generator->run();
    std::cout << result.source << std::endl;
}

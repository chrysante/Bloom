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
    MaterialGraph g({ .shadingModel = MaterialShadingModel::DefaultLit });
    auto* tex0 =
        g.addValue<TextureParameter>("TexParam",
                                     ctx.textureType(TextureSubType::Texture2D),
                                     genTexture());
    auto* tex1 = ctx.textureConstant(TextureSubType::TextureCube, genTexture());
    auto* const0 =
        g.addValue<VectorParameter>(ctx, "Param0", 2, float4{ 1, 0, 0, 0 });
    auto* const1 =
        g.addValue<VectorParameter>(ctx, "Param1", 3, float4{ 2, 1, 5, 0 });
    auto* const2 =
        g.addValue<VectorParameter>(ctx, "Param2", 2, float4{ 2, 1, 5, 0 });
    auto* vec0 =
        g.addValue<VectorBuilder>(ctx, "value",
                                  std::vector<Value*>{
                                      ctx.vectorConstant(1, { 1, 0, 0, 0 }),
                                      ctx.vectorConstant(1, { 2, 0, 0, 0 }),
                                      ctx.vectorConstant(1, { 3, 0, 0, 0 }),
                                  });
    auto* coords = ctx.rasterizerInput(RasterizerInputKind::TextureCoords);
    auto* sample0 = g.addValue<TextureSampler>(ctx, "sample", tex0, coords,
                                               SampleMode::Normal);
    auto* sample1 = g.addValue<TextureSampler>(ctx, "sample", tex1, coords,
                                               SampleMode::Color);
    auto* aggType = ctx.aggregateType("agg_type", { { const0->type(), "A" },
                                                    { const1->type(), "B" },
                                                    { vec0->type(), "C" },
                                                    { sample0->type(), "D" },
                                                    { sample1->type(), "E" },
                                                    { const2->type(), "F" } });
    auto* aggregate =
        g.addValue<AggregateBuilder>("agg_value", aggType,
                                     std::vector<Value*>{ const0, const1, vec0,
                                                          sample0, sample1,
                                                          const2 });
    Value* resultVal = g.addValue<ExtractAggregateValue>("elem", aggregate, 1);
    resultVal = g.addValue<FunctionCall>(ctx, "value",
                                         BuiltinFunction::Normalize,
                                         std::vector<Value*>{ resultVal });
    g.setRoot(GraphRoot::LitFragmentFunction, resultVal);
    g.buildFunctions(ctx);
    auto generator = makeCodeGenerator(ctx, g, {});
    auto result = generator->run();
    std::cout << result.source << std::endl;
}

TEST_CASE("Builtins") {
    Context ctx;
    MaterialGraph g({ .shadingModel = MaterialShadingModel::Unlit });
    using enum BuiltinFunction;
    SECTION("Cross with invalid arguments") {
        CHECK_THROWS(g.addValue<FunctionCall>(ctx, "value", Cross,
                                              std::vector<Value*>{
                                                  ctx.float2Constant({}),
                                                  ctx.float3Constant({}) }));
    }
    SECTION("Cross with valid arguments") {
        auto* call = g.addValue<FunctionCall>(ctx, "value", Cross,
                                              std::vector<Value*>{
                                                  ctx.float3Constant({}),
                                                  ctx.float3Constant({}) });
        CHECK(call->type() == ctx.float3Type());
    }
    SECTION("Reflect with invalid arguments") {
        CHECK_THROWS(g.addValue<FunctionCall>(ctx, "value", Reflect,
                                              std::vector<Value*>{
                                                  ctx.float2Constant({}),
                                                  ctx.float3Constant({}) }));
    }
    SECTION("Reflect with valid arguments") {
        auto* call = g.addValue<FunctionCall>(ctx, "value", Reflect,
                                              std::vector<Value*>{
                                                  ctx.float3Constant({}),
                                                  ctx.float3Constant({}) });
        CHECK(call->type() == ctx.float3Type());
    }
}

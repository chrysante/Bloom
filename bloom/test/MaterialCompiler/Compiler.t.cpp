#include <cstdlib>
#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <range/v3/algorithm.hpp>

#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/MaterialCompiler/BuildModule.h"
#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/Module.h"

using namespace bloom;
using namespace matc;
using namespace vml::short_types;

static AssetHandle genTexture() {
    return AssetHandle(AssetType::Texture2D, utl::uuid::generate());
}

static void line(std::string_view msg, std::ostream& str) {
    size_t const width = 80;
    auto impl = [&](size_t width) {
        for (size_t i = 0; i < width; ++i) {
            str << "=";
        }
    };
    if (msg.empty()) {
        impl(width);
        str << std::endl;
    }
    else if (width <= msg.size() + 2) {
        str << msg << std::endl;
    }
    else {
        size_t outerSpace = width - (msg.size() + 2);
        size_t left = outerSpace / 4, right = outerSpace - left;
        impl(left);
        str << " " << msg << " ";
        impl(right);
        str << std::endl;
    }
}

static void header(std::string_view title) {
    std::cout << "\n";
    line("", std::cout);
    line(title, std::cout);
    line("", std::cout);
    std::cout << "\n";
}

static void subHeader(std::string_view title) {
    std::cout << "\n";
    line(title, std::cout);
    std::cout << "\n";
}

static void printTexParam(MaterialTextureParameter const& param) {
    subHeader(param.name);
    std::cout << "Type:          " << magic_enum::enum_name(param.type)
              << std::endl;
    std::cout << "BindingPoints: \n";
    for (auto shaderType: magic_enum::enum_values<MaterialShaderType>()) {
        std::cout << "    " << magic_enum::enum_name(shaderType) << ": "
                  << param.bindingPoint[shaderType] << std::endl;
    }
    std::cout << "Default Value: " << param.value.ID() << std::endl;
}

static void printPrimParam(MaterialPrimitiveParameter const& param) {
    subHeader(param.name);
    std::cout << "Type:          " << magic_enum::enum_name(param.type)
              << std::endl;
    std::cout << "Buffer Offset: " << param.bufferOffset << std::endl;
    std::cout << "Default Value: " << param.value << std::endl;
}

static void printSourceData(MaterialSourceData const& sourceData) {
    header("Source");
    std::cout << sourceData.source << std::endl;
    header("Texture Constants Parameters");
    ranges::for_each(sourceData.textureConstants, printTexParam);
    header("Texture Parameters");
    ranges::for_each(sourceData.textureParameters, printTexParam);
    header("Vector Parameters");
    ranges::for_each(sourceData.primitiveParameters, printPrimParam);
    header("Misc");
    std::cout << "Param Buffer Size: " << sourceData.parameterBufferSize
              << std::endl;
    std::cout << "Cull Mode: " << magic_enum::enum_name(sourceData.cullMode)
              << std::endl;
}

TEST_CASE() {
    Context ctx;
    CodegenParameters codegenParams = {
        .shadingModel = MaterialShadingModel::DefaultLit,
        .transparency = MaterialTransparency::Masked
    };
    MaterialGraph g;
    auto* tex0 = g.addTextureParameter(ctx, "TexParam",
                                       TextureSubType::Texture2D, genTexture());
    auto* tex1 = ctx.textureConstant(TextureSubType::TextureCube, genTexture());
    auto* const0 = g.addVectorParameter(ctx, "Param0", 2, float4{ 1, 0, 0, 0 });
    auto* const1 = g.addVectorParameter(ctx, "Param1", 3, float4{ 2, 1, 5, 0 });
    auto* const2 = g.addVectorParameter(ctx, "Param2", 2, float4{ 2, 1, 5, 0 });
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
                                         BuiltinFunctionKind::Normalize,
                                         std::vector<Value*>{ resultVal });
    g.setRoot(GraphRoot::ShadingFunction, resultVal);
    auto* transparency = g.addValue<ExtractVectorValue>(ctx, "R", sample0, 0);
    g.setRoot(GraphRoot::TransparencyFunction, transparency);
    MaterialSourceData sourceData;
    auto mod = buildModule(ctx, g, codegenParams);
    auto generator = makeCodeGenerator(ctx, mod);
    sourceData.source = generator->run();
    printSourceData(sourceData);
}

TEST_CASE() {
    return;
    //    Context ctx;
    //    MaterialGraph g({ .shadingModel = MaterialShadingModel::DefaultLit });
    //    auto* UV = g.addValue<RasterizerInput>(ctx, "UV",
    //                                           RasterizerInputKind::TextureCoords);
    //    g.setRoot(GraphRoot::ShadingFunction, UV);
    //    auto sourceData = g.generate(ctx);
    //    printSourceData(sourceData);
}

TEST_CASE("Builtins") {
    Context ctx;
    MaterialGraph g;
    using enum BuiltinFunctionKind;
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

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
    auto* AOTexture =
        ctx.textureConstant(TextureSubType::TextureCube, genTexture());
    auto* normalTexture =
        ctx.textureConstant(TextureSubType::TextureCube, genTexture());
    auto* albedo = g.addVectorParameter(ctx, "Albedo", 3, float4{ 1, 0, 0, 0 });
    auto* roughness =
        g.addVectorParameter(ctx, "Roughness", 1, float4{ 2, 1, 5, 0 });
    auto* emissive =
        g.addValue<VectorBuilder>(ctx, "Emissive",
                                  std::vector<Value*>{
                                      ctx.vectorConstant(1, { 1, 0, 0, 0 }),
                                      ctx.vectorConstant(1, { 2, 0, 0, 0 }),
                                      ctx.vectorConstant(1, { 3, 0, 0, 0 }),
                                  });
    auto* metallic =
        g.addVectorParameter(ctx, "Metallic", 1, float4{ 2, 1, 5, 0 });
    auto* coords = ctx.rasterizerInput(RasterizerInputKind::TextureCoords);
    auto* AOSample = g.addValue<TextureSampler>(ctx, "AOSample", AOTexture,
                                                coords, SampleMode::Color);
    auto* AO = g.addValue<ExtractVectorValue>(ctx, "AO", AOSample, 0);
    auto* normalSample = g.addValue<TextureSampler>(ctx, "NormalSample",
                                                    normalTexture, coords,
                                                    SampleMode::Normal);
    auto* normal =
        g.addValue<ExtractVectorValue>(ctx, "Normal", normalSample, 0, 1, 2);
    auto* normal2 = g.addValue<FunctionCall>(ctx, "Normal2",
                                             BuiltinFunctionKind::Normalize,
                                             std::vector<Value*>{ normal });
    auto* materialType = makeDefaultLitMaterialType(ctx);
    auto* material =
        g.addValue<AggregateBuilder>("material", materialType,
                                     std::vector<Value*>{ albedo, roughness,
                                                          emissive, metallic,
                                                          AO, normal2 });
    g.setRoot(GraphRoot::ShadingFunction, material);
    auto* transparency = g.addValue<ExtractVectorValue>(ctx, "R", AOSample, 1);
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

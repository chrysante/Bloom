#include "Bloom/MaterialCompiler/BuildModule.h"

#include <charconv>
#include <regex>

#include <magic_enum.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/EnumCount.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Core/Utils.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/FunctionBuilder.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/Module.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

[[noreturn]] static void unreachable() { __builtin_unreachable(); }

namespace {

enum class VertexAttribKind { Position, Direction };

enum class ParameterBufferTag : int;

struct ParameterMetadata:
    std::variant<RasterizerInput*, TextureConstant*, ParameterBufferTag> {
    using variant::variant;
};

struct FunctionMetadata {
    utl::hashset<RasterizerInput*> usedRasterizerInputs;
    utl::hashmap<FunctionParameter*, ParameterMetadata> parameters;
    FunctionParameter* primitiveBufferParam = nullptr;
    FunctionParameter* rasterizerParam = nullptr;
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
};

enum class ShadingScenario { Shading, Shadow };

struct ScenarioMetadata {
    utl::hashset<RasterizerInput*> usedRasterizerData;
    AggregateType const* rasterizerDataType;
    EnumArray<RasterizerInputKind, int> rasterizerDataIndexMap;
    bool hasCustomNormals = true;

    bool isUsed(RasterizerInput const* input) const {
        return usedRasterizerData.contains(input);
    }
    bool isUsed(RasterizerInputKind kind) const {
        return ranges::contains(usedRasterizerData, kind,
                                &RasterizerInput::kind);
    }
};

struct CloneMap {
    void insert(Instruction const* original, Instruction* copy) {
        map.insert({ original, copy });
    }

    Instruction* get(Instruction* inst) const {
        auto itr = map.find(inst);
        assert(itr != map.end());
        return itr->second;
    }

    Value* get(Value* value) const {
        if (auto* inst = dyncast<Instruction*>(value)) {
            return get(inst);
        }
        return value;
    }

private:
    utl::hashmap<Instruction const*, Instruction*> map;
};

struct Builder {
    Context& ctx;
    MaterialGraph const& G;
    CodegenParameters codegenParams;
    Module& mod;

    /// Functions defined by the material editor and shared between shaders
    utl::hashmap<GraphRoot, Function*> userFunctions = {};

    /// Different shaders exposed to the renderer
    utl::hashmap<MaterialShaderType, Function*> shaderFunctions = {};

    /// Metadata stored for each function
    utl::hashmap<Function*, FunctionMetadata> functionMetadata = {};

    /// Metadata stored for each shading scenario
    utl::hashmap<ShadingScenario, ScenarioMetadata> scenarioMetadata = {};

    /// Type of the vector parameter buffer
    AggregateType const* primitiveBufferType = nullptr;
    AggregateType const* cameraInputType = nullptr;
    AggregateType const* vertex3DType = nullptr;
    Type const* entityIDType = nullptr;
    AggregateType const* shadingFragmentOutType = nullptr;

    void run();
    void verifyMaterial();
    void declareTypes();
    void buildUserFunctions();
    DynUniquePtr<Function> buildUserFunction(std::string name, Value* root,
                                             utl::hashset<Value*>& usedParams);
    void analyzeRasterizerUsage(Function& F,
                                utl::hashset<RasterizerInput*>& usedInputs);
    void rewriteRasterizerUsage(Function& F, FunctionMetadata& metadata);
    void computeParameters(Function& F, utl::hashset<Value*> const& usedParams,
                           std::span<VectorConstant* const> vectorParameters);
    void defineTextureBindings();

    void defineShadingScenario();
    void analyzeShadingFunction(Function const& F, ScenarioMetadata& MD);
    Value* postprocessMaterial(Value* value);
    Function* declarePBRShader(Type const* materialType);
    Value* callPBRShader(FunctionBuilder& builder, Value* materialData,
                         ScenarioMetadata const& scenarioMD);
    Value* transformAttribFloat3(FunctionBuilder& builder, std::string name,
                                 Value* transformMatrix, Value* attrib,
                                 VertexAttribKind kind);
    Value* transformAttribFloat4(FunctionBuilder& builder, std::string name,
                                 Value* transformMatrix, Value* attrib);
    void uniqueNames(Function& F);

    /// # Utils

    Function* getUserFunction(GraphRoot kind) const;
    utl::small_vector<Function*> getUserFunctions(
        std::span<GraphRoot const> kinds) const;

    /// Emits a call instruction to \p builder and adds parameters to
    /// `builder.function()` as necessary to access required data
    FunctionCall* callFunction(std::string valueName, FunctionBuilder& builder,
                               Function* callee,
                               ScenarioMetadata const& scenarioMD);
};

} // namespace

Module matc::buildModule(Context& ctx, MaterialGraph const& G,
                         CodegenParameters codegenParams) {
    Module mod;
    Builder B{ ctx, G, codegenParams, mod };
    B.run();
    return mod;
}

void Builder::run() {
    verifyMaterial();
    declareTypes();
    buildUserFunctions();
    defineTextureBindings();
    defineShadingScenario();
    ranges::for_each(mod.functions(), [&](auto* F) { uniqueNames(*F); });
}

void Builder::verifyMaterial() {
    auto* shadingValue = G.getRoot(GraphRoot::ShadingFunction);
    assert(shadingValue && "Graph must have a shading function");
    switch (codegenParams.shadingModel) {
        using enum MaterialShadingModel;
    case Unlit: {
        assert(shadingValue->type() == ctx.float3Type() &&
               "Unlit shading function must return float3");
        break;
    }
    case DefaultLit:
        auto* type = dyncast<AggregateType const*>(shadingValue->type());
        assert(type == makeDefaultLitMaterialType(ctx) &&
               "Invalid return type");
        break;
    }
    switch (codegenParams.transparency) {
        using enum MaterialTransparency;
    case Opaque:
        break;
    case Masked: {
        auto* transparency = G.getRoot(GraphRoot::TransparencyFunction);
        assert(transparency &&
               "Masked material must have a transparency function");
        assert(transparency->type() == ctx.floatType() &&
               "Transparency must be a float value");
        break;
    }
    }
}

void Builder::declareTypes() {
    // clang-format off
    cameraInputType = ctx.aggregateType("CameraInput", {
        { ctx.matrixType(4, 4), "camera" },
        { ctx.matrixType(4, 4), "inverseCamera" },
        { ctx.float3Type(), "position" },
        { ctx.float3Type(), "front" },
        { ctx.float2Type(), "viewportSize" },
    }, /* isExtern: */ true);
    using enum VectorTypeFlags;
    vertex3DType = ctx.aggregateType("Vertex3D", {
        { ctx.float3Type(Packed), "position" },
        { ctx.float3Type(Packed), "normal" },
        { ctx.float3Type(Packed), "tangent" },
        { ctx.float3Type(Packed), "binormal" },
        { ctx.float4Type(), "color" },
        { ctx.arrayType(ctx.float2Type(), 
                        ctx.intConstant(IntSubType::I32, 4)),
          "textureCoordinates" },
    }, /* isExtern: */ true);
    entityIDType = ctx.intType(IntSubType::U32);
    shadingFragmentOutType = ctx.aggregateType("ShadingFragmentOut", {
        { ctx.float4Type(), "color", { ParameterAttribute::Color, 0 } },
        { entityIDType, "entityID", { ParameterAttribute::Color, 1 } }
    });
    // clang-format on
}

static AggregateType const* makeParameterBufferType(
    Context& ctx, std::span<VectorConstant const* const> params) {
    if (params.empty()) {
        return nullptr;
    }
    return ctx.aggregateType("PrimitiveParameters",
                             params | transform([&](auto* param) mutable {
        return AggregateElement{ param->type(), param->name() };
    }) | ranges::to<std::vector>);
}

static bool vectorTypeLess(VectorConstant const* a, VectorConstant const* b) {
    return a->type()->numComponents() < b->type()->numComponents();
}

void Builder::buildUserFunctions() {
    utl::hashmap<GraphRoot, utl::hashset<Value*>> materialParamMap;
    // Build functions
    for (auto [rootKind, root]: G.roots()) {
        std::string name(magic_enum::enum_name(rootKind));
        auto F = buildUserFunction(std::move(name), root,
                                   materialParamMap[rootKind]);
        userFunctions[rootKind] = mod.addFunction(std::move(F));
    }
    // Analyze rasterizer usage
    auto fragmentFunctions = getUserFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    for (auto* F: fragmentFunctions) {
        auto& metadata = functionMetadata[F];
        analyzeRasterizerUsage(*F, metadata.usedRasterizerInputs);
        rewriteRasterizerUsage(*F, metadata);
    }
    // Parameters
    auto vectorParameters = materialParamMap | values | join |
                            Filter<VectorConstant> | ranges::to<std::vector>;
    ranges::sort(vectorParameters, vectorTypeLess);
    primitiveBufferType = makeParameterBufferType(ctx, vectorParameters);
    for (auto& [functionKind, F]: userFunctions) {
        computeParameters(*F, materialParamMap[functionKind], vectorParameters);
        auto& ret = cast<ReturnInst const&>(F->back());
        auto* retType = ret.returnValue()->type();
        F->computeType(ctx, retType);
    }
}

static void traverseTopsort(Value* root,
                            utl::function_view<void(Value&)> callback) {
    utl::hashset<Value*> perm;
    utl::hashset<Value*> tmp;
    auto visit = [&](auto& visit, Value* value) {
        if (!value) {
            throw std::runtime_error("Encountered NULL value");
        }
        if (perm.contains(value)) {
            return;
        }
        if (tmp.contains(value)) {
            throw std::runtime_error("Function graph is not a DAG");
        }
        tmp.insert(value);
        if (auto* user = dyncast<User const*>(value)) {
            for (auto* operand: user->operands()) {
                visit(visit, operand);
            }
        }
        tmp.erase(value);
        perm.insert(value);
        callback(*value);
    };
    visit(visit, root);
}

DynUniquePtr<Function> Builder::buildUserFunction(
    std::string name, Value* root, utl::hashset<Value*>& usedParams) {
    CloneMap map;
    List<Instruction> body;
    traverseTopsort(root, [&](auto& value) {
        if (auto* inst = dyncast<Instruction const*>(&value)) {
            body.push_back(inst->clone().release());
            auto* clone = &body.back();
            map.insert(inst, clone);
        }
    });
    body.push_back(new ReturnInst(ctx, "ret", root));
    for (auto& inst: body) {
        for (auto [index, operand]: inst.operands() | enumerate) {
            if (auto* instOperand = dyncast<Instruction*>(operand)) {
                inst.setOperand(index, map.get(instOperand));
            }
            if (G.userParameters().contains(operand) ||
                isa<TextureConstant>(operand))
            {
                usedParams.insert(operand);
            }
        }
    }
    return allocate<Function>(std::move(name), nullptr, ShaderKind::None,
                              List<FunctionParameter>{}, std::move(body));
}

void Builder::analyzeRasterizerUsage(
    Function& F, utl::hashset<RasterizerInput*>& usedInputs) {
    for (auto& inst: F) {
        for (auto* input: inst.operands() | Filter<RasterizerInput>) {
            usedInputs.insert(input);
        }
    }
}

static Value* makeParameter(Function& F, Value* param,
                            AggregateType const* paramBufferType,
                            std::span<VectorConstant* const> vectorParameters,
                            FunctionMetadata& metadata) {
    // clang-format off
    return visit(*param, utl::overload{
        [&](VectorConstant const& matParam) {
            if (!metadata.primitiveBufferParam) {
                auto* P = F.addParameter({
                    "primitives", paramBufferType,
                    ParameterQualifier::DeviceConstRef});
                metadata.parameters[P] = ParameterBufferTag{};
                metadata.primitiveBufferParam = P;
            }
            auto itr = ranges::find(vectorParameters, &matParam);
            int index = utl::narrow_cast<int>(itr - vectorParameters.begin());
            auto* value = new ExtractAggregateValue(matParam.name(),
                                                    metadata.primitiveBufferParam, index);
            return F.push_front(value);
        },
        [&](TextureConstant& matParam) {
            auto* P = F.addParameter(
                                     {matParam.name(), matParam.type()});
            metadata.parameters[P] = &matParam;
            return P;
        },
        [&](Value const&) -> Value* { unreachable(); }
    });
    // clang-format on
}

void Builder::computeParameters(
    Function& F, utl::hashset<Value*> const& usedParams,
    std::span<VectorConstant* const> vectorParameters) {
    auto& metadata = functionMetadata[&F];
    for (auto* param: usedParams) {
        Value* newParam = makeParameter(F, param, primitiveBufferType,
                                        vectorParameters, metadata);
        auto paramUsers = param->users() | ToSmallVector<>;
        for (auto* user: paramUsers) {
            if (auto* inst = dyncast<Instruction*>(user);
                inst && inst->function() == &F)
            {
                inst->replaceOperand(param, newParam);
            }
        }
    }
}

void Builder::rewriteRasterizerUsage(Function& F, FunctionMetadata& metadata) {
    utl::hashmap<Function const*,
                 EnumArray<RasterizerInputKind, FunctionParameter*>>
        paramMap;
    for (auto* input: metadata.usedRasterizerInputs) {
        for (auto* inst: input->users() | Filter<Instruction> | ToSmallVector<>)
        {
            if (inst->function() != &F) {
                continue;
            }
            auto& param = paramMap[&F][input->kind()];
            if (!param) {
                std::string name(magic_enum::enum_name(input->kind()));
                param = F.addParameter({ std::move(name), input->type() });
                metadata.parameters[param] = input;
            }
            inst->replaceOperand(input, param);
        }
    }
}

void Builder::defineTextureBindings() {
    static constexpr int ReservedTextureSlots = 8;
    int textureBindingPoint = ReservedTextureSlots;
    for (auto* tex: ctx.textureConstants()) {
        if (tex->users().empty()) {
            continue;
        }
        int bindingPoint = textureBindingPoint++;
        tex->setBindingPoint(bindingPoint);
    }
}

static ParameterAttribute toAttrib(RasterizerInputKind kind) {
    using enum RasterizerInputKind;
    switch (kind) {
    case ClipPosition:
        return { ParameterAttribute::Position };
    default:
        return {};
    }
}

static ParameterAttribute getGlobalBindingAttrib(
    MaterialShaderType shader, GlobalMaterialBindingType binding) {
    auto [type, index] = getGlobalMaterialBindingPoint(shader, binding);
    return { [&] {
        switch (type) {
        case MaterialBindingPoint::Buffer:
            return ParameterAttribute::Buffer;
        case MaterialBindingPoint::Texture:
            return ParameterAttribute::Texture;
        }
    }(), index };
}

static void makeRasterizerMetadata(Context& ctx, std::string typeName,
                                   ScenarioMetadata& scenarioMD) {
    if (scenarioMD.usedRasterizerData.empty()) {
        return;
    }
    auto* type =
        ctx.aggregateType(std::move(typeName), scenarioMD.usedRasterizerData |
                                                   transform([&](auto* value) {
        return AggregateElement{ value->type(),
                                 std::string(toSpelling(value->kind())),
                                 toAttrib(value->kind()) };
    }) | ranges::to<std::vector>);
    auto map = makeEnumArray<RasterizerInputKind>(-1);
    for (auto [index, value]: scenarioMD.usedRasterizerData | enumerate) {
        map[value->kind()] = utl::narrow_cast<int>(index);
    }
    scenarioMD.rasterizerDataType = type;
    scenarioMD.rasterizerDataIndexMap = map;
}

static Value* getRasterizerInput(std::string name, FunctionBuilder& builder,
                                 RasterizerInputKind kind, FunctionMetadata& MD,
                                 ScenarioMetadata const& scenarioMD) {
    return builder
        .insert<ExtractAggregateValue>(std::move(name), MD.rasterizerParam,
                                       scenarioMD.rasterizerDataIndexMap[kind]);
}

void Builder::defineShadingScenario() {
    // Fragment Shader
    auto& fragShader =
        *mod.addFunction("FragmentShading", nullptr, ShaderKind::Fragment);
    shaderFunctions[MaterialShaderType::ShadingFragment] = &fragShader;
    auto& fragMD = functionMetadata[&fragShader];
    auto& scenarioMD = scenarioMetadata[ShadingScenario::Shading];
    auto userFunctions = getUserFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    ranges::copy(userFunctions | transform([&](auto* F) -> auto& {
        auto& MD = functionMetadata[F];
        return MD.usedRasterizerInputs;
    }) | join,
                 std::inserter(scenarioMD.usedRasterizerData,
                               scenarioMD.usedRasterizerData.begin()));
    scenarioMD.usedRasterizerData.insert(
        ctx.rasterizerInput(RasterizerInputKind::ClipPosition));
    if (codegenParams.shadingModel == MaterialShadingModel::DefaultLit) {
        scenarioMD.usedRasterizerData.insert(
            ctx.rasterizerInput(RasterizerInputKind::WorldPosition));
    }
    Function* shadingFunction = getUserFunction(GraphRoot::ShadingFunction);
    analyzeShadingFunction(*shadingFunction, scenarioMD);
    makeRasterizerMetadata(ctx, "ShadingRasterizerData", scenarioMD);
    fragMD.rasterizerParam = fragShader.addParameter(
        { "in", scenarioMD.rasterizerDataType, ParameterQualifier::None,
          ParameterAttribute::StageIn });
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
    auto builder = FunctionBuilder(ctx, fragShader);
    if (codegenParams.transparency == MaterialTransparency::Masked) {
        auto* transparencyFunction =
            getUserFunction(GraphRoot::TransparencyFunction);
        assert(transparencyFunction);
        auto* transparency = callFunction("transparency", builder,
                                          transparencyFunction, scenarioMD);
        auto* condition =
            builder.insert<CompareInst>(ctx, "condition", CompareInst::Less,
                                        transparency, ctx.floatConstant(0.5));
        builder.insert<DiscardFragment>("discard", condition);
    }
    assert(shadingFunction);
    Value* materialData =
        callFunction("material", builder, shadingFunction, scenarioMD);
    materialData = postprocessMaterial(materialData);
    Value* color3 = [&]() -> Value* {
        switch (codegenParams.shadingModel) {
        case MaterialShadingModel::DefaultLit:
            return callPBRShader(builder, materialData, scenarioMD);
        case MaterialShadingModel::Unlit:
            return materialData;
        }
    }();
    auto* colorOut =
        builder.insert<VectorBuilder>("colorOut",
                                      std::vector<Value*>{
                                          color3, ctx.floatConstant(1) });
    auto entityIDAttrib =
        getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                               GlobalMaterialBindingType::GeometryIDBuffer);
    auto* entityID = fragShader.addParameter(
        { "entityID", entityIDType, ParameterQualifier::DeviceConstRef,
          entityIDAttrib });
    auto* FSRetval = builder.makeAggregate("retval", shadingFragmentOutType,
                                           { colorOut, entityID });
    builder.insert<ReturnInst>("ret", FSRetval);
    fragShader.computeType(ctx, shadingFragmentOutType);

    // Vertex Shader
    auto& vertShader =
        *mod.addFunction("VertexShading", nullptr, ShaderKind::Vertex);
    shaderFunctions[MaterialShaderType::ShadingVertex] = &vertShader;
    auto& vertMD = functionMetadata[&vertShader];
    auto* cameraBuffer = vertShader.addParameter(
        { .name = "cameraBuffer",
          .type = cameraInputType,
          .qualifier = ParameterQualifier::DeviceConstRef,
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingVertex,
                                     GlobalMaterialBindingType::Camera) });
    auto* cameraMatrix = builder.getElem("camera", cameraBuffer, 0);
    auto* vertexBuffer = vertShader.addParameter(
        { .name = "vertexBuffer",
          .type = ctx.pointerType(vertex3DType, Mutability::Const),
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingVertex,
                                     GlobalMaterialBindingType::Geometry) });
    auto* transformMatrix = vertShader.addParameter(
        { .name = "transform",
          .type = ctx.matrixType(4, 4),
          .qualifier = ParameterQualifier::DeviceConstRef,
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingVertex,
                                     GlobalMaterialBindingType::Transform) });
    auto* vertexID =
        vertShader.addParameter({ .name = "vertexID",
                                  .type = ctx.intType(IntSubType::U32),
                                  .attribute = ParameterAttribute::VertexID });
    builder = FunctionBuilder(ctx, vertShader);
    auto* vertex =
        builder.insert<ArrayAccess>("vertex", vertexBuffer, vertexID);
    EnumArray<RasterizerInputKind, Value*> rasterizerValues{};
    using enum RasterizerInputKind;
    rasterizerValues[WorldPosition] =
        transformAttribFloat3(builder, "posWS", transformMatrix,
                              builder.getElem("vPosition", vertex, 0),
                              VertexAttribKind::Position);
    rasterizerValues[ClipPosition] =
        transformAttribFloat4(builder, "posCS", cameraMatrix,
                              rasterizerValues[WorldPosition]);
    if (scenarioMD.isUsed(WorldNormals)) {
        rasterizerValues[WorldNormals] =
            transformAttribFloat3(builder, "normalWS", transformMatrix,
                                  builder.getElem("vNormal", vertex, 1),
                                  VertexAttribKind::Direction);
    }
    if (scenarioMD.isUsed(WorldTangents)) {
        rasterizerValues[WorldTangents] =
            transformAttribFloat3(builder, "tangentWS", transformMatrix,
                                  builder.getElem("vTangent", vertex, 2),
                                  VertexAttribKind::Direction);
    }
    if (scenarioMD.isUsed(WorldBinormals)) {
        rasterizerValues[WorldBinormals] =
            transformAttribFloat3(builder, "binormalWS", transformMatrix,
                                  builder.getElem("vBinormal", vertex, 3),
                                  VertexAttribKind::Direction);
    }
    if (scenarioMD.isUsed(VertexColor)) {
        rasterizerValues[VertexColor] =
            builder.getElem("vertexColor", vertex, 4);
    }
    if (scenarioMD.isUsed(TextureCoords)) {
        auto* array = builder.getElem("coords", vertex, 5);
        rasterizerValues[TextureCoords] =
            builder.insert<ArrayAccess>("elem", array,
                                        ctx.intConstant(IntSubType::I32, 0));
    }
    auto usedRasterData = scenarioMD.usedRasterizerData |
                          transform([](auto* input) { return input->kind(); }) |
                          ToSmallVector<>;
    ranges::sort(usedRasterData, ranges::less{},
                 [](auto value) { return (int)value; });
    auto* VSRetval =
        builder.makeAggregate("retval", scenarioMD.rasterizerDataType,
                              usedRasterData | transform([&](auto kind) {
        auto* value = rasterizerValues[kind];
        assert(value);
        return value;
    }) | ranges::to<std::vector>);
    builder.insert<ReturnInst>("ret", VSRetval);
    vertShader.computeType(ctx, scenarioMD.rasterizerDataType);
}

void Builder::analyzeShadingFunction(Function const& F, ScenarioMetadata& MD) {
    if (codegenParams.shadingModel != MaterialShadingModel::DefaultLit) {
        return;
    }
    auto* ret = F.getReturn();
    assert(ret && "Function has no return instruction");
    if (auto* value = dyncast<AggregateBuilder const*>(ret->returnValue())) {
        auto* normals = value->element((size_t)DefaultLitMaterialValue::Normal);
        if (normals == ctx.float3Constant({ 0, 0, 1 })) {
            MD.hasCustomNormals = false;
        }
    }
    using enum RasterizerInputKind;
    MD.usedRasterizerData.insert(ctx.rasterizerInput(WorldNormals));
    if (MD.hasCustomNormals) {
        MD.usedRasterizerData.insert(ctx.rasterizerInput(WorldTangents));
        MD.usedRasterizerData.insert(ctx.rasterizerInput(WorldBinormals));
    }
}

Value* Builder::postprocessMaterial(Value* value) {
    switch (codegenParams.shadingModel) {
    case MaterialShadingModel::Unlit:
        // TODO: Clamp to min zero
        return value;
    case MaterialShadingModel::DefaultLit: {
        // TODO: Clamp albeda and emissive to min zero and normalize normals
        return value;
    }
    }
}

Value* Builder::callPBRShader(FunctionBuilder& builder, Value* materialData,
                              ScenarioMetadata const& scenarioMD) {
    Function* pbrShader = declarePBRShader(materialData->type());
    Function& shader = *builder.function();
    auto& MD = functionMetadata[&shader];
    auto* worldPosition = getRasterizerInput("worldPosition", builder,
                                             RasterizerInputKind::WorldPosition,
                                             MD, scenarioMD);
    auto* cameraBuffer = shader.addParameter(
        { .name = "cameraBuffer",
          .type = cameraInputType,
          .qualifier = ParameterQualifier::DeviceConstRef,
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                     GlobalMaterialBindingType::Camera) });
    auto* cameraPosition =
        builder.insert<ExtractAggregateValue>("cameraPosition", cameraBuffer,
                                              2);
    auto* lightBuffer = shader.addParameter(
        { .name = "lights",
          .type = ctx.aggregateType("LightBufferContents", {},
                                    /* isExtern: */ true),
          .qualifier = ParameterQualifier::DeviceConstRef,
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                     GlobalMaterialBindingType::Lights) });
    auto* lightSpaceTransforms = shader.addParameter(
        { .name = "lightSpaceTransforms",
          .type = ctx.pointerType(ctx.matrixType(4, 4), Mutability::Const),
          .attribute = getGlobalBindingAttrib(
              MaterialShaderType::ShadingFragment,
              GlobalMaterialBindingType::LightSpaceTransforms) });
    auto* shadowMaps = shader.addParameter(
        { .name = "shadowMaps",
          .type = ctx.textureType(TextureSubType::Texture2DArray),
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                     GlobalMaterialBindingType::ShadowMaps) });
    auto* specularMap = shader.addParameter(
        { .name = "specularMap",
          .type = ctx.textureType(TextureSubType::TextureCube),
          .attribute = getGlobalBindingAttrib(
              MaterialShaderType::ShadingFragment,
              GlobalMaterialBindingType::SpecularTexture) });
    auto* bdrfMap = shader.addParameter(
        { .name = "bdrfMap",
          .type = ctx.textureType(TextureSubType::Texture2D),
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                     GlobalMaterialBindingType::BRDFTexture) });
    return builder.callFunction("color", pbrShader,
                                { { materialData, worldPosition, cameraPosition,
                                    lightSpaceTransforms, lightBuffer,
                                    shadowMaps, specularMap, bdrfMap } });
}

static DynUniquePtr<Function> makeForwardDecl(
    Context& ctx, std::string name, ShaderKind kind, Type const* returnType,
    std::span<ParameterDescriptor const> params) {
    auto F = allocate<Function>(std::move(name), nullptr, kind,
                                allocateParameters(params));
    F->computeType(ctx, returnType);
    return F;
}

Function* Builder::declarePBRShader(Type const* materialType) {
    std::array<ParameterDescriptor, 8> params = { {
        { .type = materialType },
        { .type = ctx.float3Type() },
        { .type = ctx.float3Type() },
        { .type = ctx.pointerType(ctx.matrixType(4, 4), Mutability::Const) },
        { .type = ctx.aggregateType("LightBufferContents", {},
                                    /* isExtern: */ true) },
        { .type = ctx.textureType(TextureSubType::Texture2DArray) },
        { .type = ctx.textureType(TextureSubType::TextureCube) },
        { .type = ctx.textureType(TextureSubType::Texture2D) },
    } };
    auto decl = makeForwardDecl(ctx, "PBRForwardShader", ShaderKind::Extern,
                                ctx.float3Type(), params);
    return mod.addFunction(std::move(decl));
}

Value* Builder::transformAttribFloat3(FunctionBuilder& builder,
                                      std::string name, Value* transformMatrix,
                                      Value* attrib, VertexAttribKind kind) {
    auto* vec =
        builder.appendVector("tmp",
                             { attrib,
                               ctx.floatConstant(
                                   kind == VertexAttribKind::Position ? 1 :
                                                                        0) });
    return transformAttribFloat4(builder, std::move(name), transformMatrix,
                                 vec);
}

Value* Builder::transformAttribFloat4(FunctionBuilder& builder,
                                      std::string name, Value* transformMatrix,
                                      Value* attrib) {
    return builder.insert<ArithmeticInst>(name, ArithmeticInst::Mul,
                                          transformMatrix, attrib);
}

void Builder::uniqueNames(Function& F) {
    auto values =
        concat(F.parameters(),
               F | transform([](auto& value) -> Value& { return value; }));
    utl::hashset<std::string> set;
    static std::regex const R("_\\d+$");
    for (auto& value: values) {
        while (!set.insert(value.name()).second) {
            auto name = value.name();
            std::smatch result;
            if (!std::regex_search(name, result, R)) {
                name += "_0";
                value.setName(std::move(name));
                continue;
            }
            assert(result.size() >= 1);
            int counter = -1;
            auto [begin, end] = std::pair{ result[0] };
            auto convResult = std::from_chars(std::to_address(begin + 1),
                                              std::to_address(end), counter);
            assert(convResult.ec == std::errc{});
            assert(counter >= 0);
            name.replace(begin + 1, end, std::to_string(counter + 1));
            value.setName(std::move(name));
        }
    }
}

Function* Builder::getUserFunction(GraphRoot kind) const {
    auto itr = userFunctions.find(kind);
    return itr != userFunctions.end() ? itr->second : nullptr;
}

utl::small_vector<Function*> Builder::getUserFunctions(
    std::span<GraphRoot const> kinds) const {
    utl::small_vector<Function*> result;
    for (auto kind: kinds) {
        if (auto* F = getUserFunction(kind)) {
            result.push_back(F);
        }
    }
    return result;
}

namespace {

struct ArgumentContext {
    FunctionBuilder& builder;
    ScenarioMetadata const& scenarioMD;
    FunctionMetadata& callerMD;
    FunctionMetadata& calleeMD;
    MaterialShaderType shaderType;
    Type const* primitiveBufferType;
};

} // namespace

static Value* getFunctionArgument(RasterizerInput* input, ArgumentContext AC) {
    auto kind = input->kind();
    std::string name(magic_enum::enum_name(kind));
    return getRasterizerInput(std::move(name), AC.builder, kind, AC.callerMD,
                              AC.scenarioMD);
}

static Value* getFunctionArgument(TextureConstant* texParam,
                                  ArgumentContext AC) {
    auto itr = AC.callerMD.texParamMap.find(texParam);
    if (itr != AC.callerMD.texParamMap.end()) {
        return itr->second;
    }
    ParameterAttribute attrib = { ParameterAttribute::Texture,
                                  texParam->bindingPoint() };
    auto* caller = AC.builder.function();
    auto* param = caller->addParameter({ texParam->name(), texParam->type(),
                                         ParameterQualifier::None, attrib });
    AC.callerMD.texParamMap.insert({ texParam, param });
    return param;
}

static Value* getFunctionArgument(ParameterBufferTag, ArgumentContext AC) {
    auto& param = AC.callerMD.primitiveBufferParam;
    if (param) {
        return param;
    }
    auto attrib = getGlobalBindingAttrib(
        AC.shaderType, GlobalMaterialBindingType::MaterialParameterBuffer);
    auto* caller = AC.builder.function();
    param =
        caller->addParameter({ "primitives", AC.primitiveBufferType,
                               ParameterQualifier::DeviceConstRef, attrib });
    return param;
}

FunctionCall* Builder::callFunction(std::string valueName,
                                    FunctionBuilder& builder, Function* callee,
                                    ScenarioMetadata const& scenarioMD) {
    auto* caller = builder.function();
    auto& callerMD = functionMetadata[caller];
    auto& calleeMD = functionMetadata[callee];
    auto arguments = callee->parameters() |
                     transform([&](FunctionParameter const& param) {
        auto itr = calleeMD.parameters.find(&param);
        assert(itr != calleeMD.parameters.end());
        return std::visit([&](auto paramMetadata) {
            ArgumentContext args = {
                .builder = builder,
                .scenarioMD = scenarioMD,
                .callerMD = callerMD,
                .calleeMD = calleeMD,
                .shaderType = MaterialShaderType::ShadingFragment,
                .primitiveBufferType = primitiveBufferType,
            };
            return getFunctionArgument(paramMetadata, args);
        }, itr->second);
    }) | ToSmallVector<>;
    return builder.callFunction(std::move(valueName), callee, arguments);
}

#include "Bloom/MaterialCompiler/BuildModule.h"

#include <magic_enum.hpp>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <utl/overload.hpp>

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

struct RasterizerMetadata2 {
    AggregateType const* type;
    EnumArray<RasterizerInputKind, int> indexMap;
};

enum class ParameterBufferTag2 : int;

struct ParameterMetadata2:
    std::variant<RasterizerInput*, TextureConstant*, ParameterBufferTag2> {
    using variant::variant;
};

struct FunctionMetadata2 {
    utl::hashset<RasterizerInput*> usedRasterizerInputs;
    utl::hashmap<FunctionParameter*, ParameterMetadata2> parameters;
    FunctionParameter* primitiveBufferParam = nullptr;
    FunctionParameter* rasterizerParam = nullptr;
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
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
    utl::hashmap<Function*, FunctionMetadata2> functionMetadata = {};

    /// Type of the vector parameter buffer
    AggregateType const* primitiveBufferType = nullptr;

    void run();
    void buildUserFunctions();
    DynUniquePtr<Function> buildUserFunction(std::string name, Value* root,
                                             utl::hashset<Value*>& usedParams);
    void analyzeRasterizerUsage(Function& F,
                                utl::hashset<RasterizerInput*>& usedInputs);
    void rewriteRasterizerUsage(Function& F, FunctionMetadata2& metadata);
    void computeParameters(Function& F, utl::hashset<Value*> const& usedParams,
                           std::span<VectorConstant* const> vectorParameters);
    void defineTextureBindings();
    Function* declarePBRShader(Type const* materialType);
    void defineShadingScenario();

    /// # Utils

    Function* getUserFunction(GraphRoot kind) const;
    utl::small_vector<Function*> getUserFunctions(
        std::span<GraphRoot const> kinds) const;

    /// Emits a call instruction to \p builder and adds parameters to
    /// `builder.function()` as necessary to access required data
    FunctionCall* callFunction(std::string valueName, FunctionBuilder& builder,
                               Function* callee,
                               RasterizerMetadata2 const& rasterizerMetadata);
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
    buildUserFunctions();
    defineTextureBindings();
    defineShadingScenario();
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
            if (G.userParameters().contains(operand)) {
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
                            FunctionMetadata2& metadata) {
    // clang-format off
    return visit(*param, utl::overload{
        [&](VectorConstant const& matParam) {
            if (!metadata.primitiveBufferParam) {
                auto* P = F.addParameter({
                    "primitives", paramBufferType,
                    ParameterQualifier::DeviceConstRef});
                metadata.parameters[P] = ParameterBufferTag2{};
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

void Builder::rewriteRasterizerUsage(Function& F, FunctionMetadata2& metadata) {
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

static RasterizerMetadata2 makeRasterizerMetadata(
    Context& ctx, std::string name,
    std::span<RasterizerInput const* const> values) {
    if (values.empty()) {
        return {};
    }
    auto* type = ctx.aggregateType(std::move(name),
                                   values | transform([&](auto* value) mutable {
        return AggregateElement{ value->type(),
                                 std::string(toSpelling(value->kind())),
                                 toAttrib(value->kind()) };
    }) | ranges::to<std::vector>);
    auto map = makeEnumArray<RasterizerInputKind>(-1);
    for (auto [index, value]: values | enumerate) {
        map[value->kind()] = utl::narrow_cast<int>(index);
    }
    return { type, map };
}

static Value* getRasterizerInput(
    std::string name, FunctionBuilder& builder, RasterizerInputKind kind,
    FunctionMetadata2& MD, RasterizerMetadata2 const& rasterizerMetadata) {
    return builder
        .insert<ExtractAggregateValue>(std::move(name), MD.rasterizerParam,
                                       rasterizerMetadata.indexMap[kind]);
}

void Builder::defineShadingScenario() {
    // Fragment Shader
    auto function =
        allocate<Function>("FragmentShading", nullptr, ShaderKind::Fragment);
    auto& fragShader = *mod.addFunction(std::move(function));
    shaderFunctions[MaterialShaderType::ShadingFragment] = &fragShader;
    auto& fragMD = functionMetadata[&fragShader];
    auto userFunctions = getUserFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    auto usedRasterizerData = userFunctions | transform([&](auto* F) -> auto& {
        auto& MD = functionMetadata[F];
        return MD.usedRasterizerInputs;
    }) | join | ranges::to<utl::hashset<RasterizerInput*>>;
    usedRasterizerData.insert(
        ctx.rasterizerInput(RasterizerInputKind::ClipPosition));
    if (codegenParams.shadingModel == MaterialShadingModel::DefaultLit) {
        usedRasterizerData.insert(
            ctx.rasterizerInput(RasterizerInputKind::WorldPosition));
    }
    auto rasterizerMetadata = makeRasterizerMetadata(ctx,
                                                     "ShadingRasterizerData",
                                                     usedRasterizerData);
    fragMD.rasterizerParam = fragShader.addParameter(
        { "in", rasterizerMetadata.type, ParameterQualifier::None,
          ParameterAttribute::StageIn });
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
    auto builder = FunctionBuilder(ctx, fragShader);
    if (codegenParams.transparency == MaterialTransparency::Masked) {
        auto* transparencyFunction =
            getUserFunction(GraphRoot::TransparencyFunction);
        assert(transparencyFunction);
        auto* transparency = callFunction("transparency", builder,
                                          transparencyFunction,
                                          rasterizerMetadata);
        auto* condition =
            builder.insert<CompareInst>(ctx, "condition", CompareInst::Less,
                                        transparency, ctx.floatConstant(0.5));
        builder.insert<DiscardFragment>("discard", condition);
    }
    auto* shadingFunction = getUserFunction(GraphRoot::ShadingFunction);
    assert(shadingFunction);
    auto* materialData =
        callFunction("material", builder, shadingFunction, rasterizerMetadata);
    Value* color = nullptr;
    switch (codegenParams.shadingModel) {
    case MaterialShadingModel::DefaultLit: {
        Function* pbrShader = declarePBRShader(materialData->type());
        auto* worldPosition =
            getRasterizerInput("worldPosition", builder,
                               RasterizerInputKind::WorldPosition, fragMD,
                               rasterizerMetadata);
        auto* cameraInputType =
            ctx.aggregateType("CameraInput",
                              {
                                  { ctx.matrixType(4, 4), "camera" },
                                  { ctx.matrixType(4, 4), "inverseCamera" },
                                  { ctx.float3Type(), "position" },
                                  { ctx.float3Type(), "front" },
                                  { ctx.float2Type(), "viewportSize" },
                              },
                              /* isExtern: */ true);
        auto* cameraBuffer = fragShader.addParameter(
            { .name = "cameraBuffer",
              .type = cameraInputType,
              .qualifier = ParameterQualifier::DeviceConstRef,
              .attribute =
                  getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                         GlobalMaterialBindingType::Camera) });
        auto* cameraPosition =
            builder.insert<ExtractAggregateValue>("cameraPosition",
                                                  cameraBuffer, 2);
        auto* lightBuffer = fragShader.addParameter(
            { .name = "lights",
              .type = ctx.aggregateType("LightBufferContents", {},
                                        /* isExtern: */ true),
              .qualifier = ParameterQualifier::DeviceConstPointer,
              .attribute =
                  getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                         GlobalMaterialBindingType::Lights) });
        auto* lightSpaceTransforms = fragShader.addParameter(
            { .name = "lightSpaceTransforms",
              .type = ctx.matrixType(4, 4),
              .qualifier = ParameterQualifier::DeviceConstPointer,
              .attribute = getGlobalBindingAttrib(
                  MaterialShaderType::ShadingFragment,
                  GlobalMaterialBindingType::LightSpaceTransforms) });
        auto* shadowMaps = fragShader.addParameter(
            { .name = "shadowMaps",
              .type = ctx.textureType(TextureSubType::Texture2DArray),
              .attribute = getGlobalBindingAttrib(
                  MaterialShaderType::ShadingFragment,
                  GlobalMaterialBindingType::ShadowMaps) });
        auto* specularMap = fragShader.addParameter(
            { .name = "specularMap",
              .type = ctx.textureType(TextureSubType::TextureCube),
              .attribute = getGlobalBindingAttrib(
                  MaterialShaderType::ShadingFragment,
                  GlobalMaterialBindingType::SpecularTexture) });
        auto* bdrfMap = fragShader.addParameter(
            { .name = "bdrfMap",
              .type = ctx.textureType(TextureSubType::Texture2D),
              .attribute = getGlobalBindingAttrib(
                  MaterialShaderType::ShadingFragment,
                  GlobalMaterialBindingType::BRDFTexture) });
        auto* color3 =
            builder.callFunction("color", pbrShader,
                                 { { materialData, worldPosition,
                                     cameraPosition, lightSpaceTransforms,
                                     lightBuffer, shadowMaps, specularMap,
                                     bdrfMap } });
        color =
            builder.insert<VectorBuilder>("colorOut",
                                          std::vector<Value*>{
                                              color3, ctx.floatConstant(1) });
        break;
    }
    case MaterialShadingModel::Unlit:
        assert(false);
        break;
    }
    auto* entityIDType = ctx.intType(IntType::U32);
    auto* retType = ctx.aggregateType("ShadingFragmentOut",
                                      { { ctx.float4Type(),
                                          "color",
                                          { ParameterAttribute::Color, 0 } },
                                        { entityIDType,
                                          "entityID",
                                          { ParameterAttribute::Color, 1 } } });
    auto entityIDAttrib =
        getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                               GlobalMaterialBindingType::GeometryIDBuffer);
    auto* entityID = fragShader.addParameter(
        { "entityID", entityIDType, ParameterQualifier::DeviceConstRef,
          entityIDAttrib });
    auto* retval = builder.insert<AggregateBuilder>("retval", retType,
                                                    std::vector<Value*>{
                                                        color, entityID });
    builder.insert<ReturnInst>("ret", retval);
    fragShader.computeType(ctx, retType);
    // Vertex Shader
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
        { .type = ctx.matrixType(4, 4),
          .qualifier = ParameterQualifier::DeviceConstPointer },
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
    RasterizerMetadata2 const& rasterizerMetadata;
    FunctionMetadata2& callerMD;
    FunctionMetadata2& calleeMD;
    MaterialShaderType shaderType;
    Type const* primitiveBufferType;
};

} // namespace

static Value* getFunctionArgument(RasterizerInput* input, ArgumentContext AC) {
    auto kind = input->kind();
    std::string name(magic_enum::enum_name(kind));
    return getRasterizerInput(std::move(name), AC.builder, kind, AC.callerMD,
                              AC.rasterizerMetadata);
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

static Value* getFunctionArgument(ParameterBufferTag2, ArgumentContext AC) {
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

FunctionCall* Builder::callFunction(
    std::string valueName, FunctionBuilder& builder, Function* callee,
    RasterizerMetadata2 const& rasterizerMetadata) {
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
                .rasterizerMetadata = rasterizerMetadata,
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

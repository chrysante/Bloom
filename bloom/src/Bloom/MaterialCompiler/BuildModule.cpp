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
#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/MaterialCompiler/Codegen.h"
#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Debug.h"
#include "Bloom/MaterialCompiler/FunctionBuilder.h"
#include "Bloom/MaterialCompiler/MaterialGraph.h"
#include "Bloom/MaterialCompiler/Module.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

namespace {

enum class VertexAttribKind { Position, Direction };

enum class ParameterBufferTag : int;

struct ParameterMetadata:
    std::variant<RasterizerInput*, TextureConstant*, ParameterBufferTag> {
    using variant::variant;
};

struct FunctionMetadata {
    utl::hashset<RasterizerInput*> usedVertexAttributes;
    utl::hashmap<FunctionParameter*, ParameterMetadata> parameters;
    FunctionParameter* primitiveBufferParam = nullptr;
    /// Only non-null for fragment shaders
    FunctionParameter* rasterizerParam = nullptr;
    /// Only non-null for vertex shaders
    EnumArray<RasterizerInputKind, Value*> vertexAttributes{};
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
};

enum class ShadingScenario { Shading, Shadow };

struct ScenarioMetadata {
    /// Vertex attributes used by the fragment shader. These need to be
    /// rasterized
    utl::hashset<RasterizerInput*> rasterizedVertexAttributes;
    /// Vertex attributes used by the vertex shader. These need to be computed
    /// but not necessarily rasterized
    utl::hashset<RasterizerInput*> computedVertexAttributes;
    AggregateType const* rasterizerDataType;
    EnumArray<RasterizerInputKind, int> rasterizerDataIndexMap;
    bool hasCustomNormals = true;

    bool isRasterized(RasterizerInput const* input) const {
        return rasterizedVertexAttributes.contains(input);
    }
    bool isRasterized(RasterizerInputKind kind) const {
        return ranges::contains(rasterizedVertexAttributes, kind,
                                &RasterizerInput::kind);
    }
    bool isComputed(RasterizerInput const* input) const {
        return computedVertexAttributes.contains(input);
    }
    bool isComputed(RasterizerInputKind kind) const {
        return ranges::contains(computedVertexAttributes, kind,
                                &RasterizerInput::kind);
    }
    void markRasterized(RasterizerInput* input) {
        rasterizedVertexAttributes.insert(input);
        computedVertexAttributes.insert(input);
    }
    void collectRasterizedVertexAttributes(auto&& functionMetadata);
    void collectComputedVertexAttributes(auto&& functionMetadata);
    void createRasterizerDataType(Context& ctx, std::string typeName);
};

struct CloneMap {
    void insert(Instruction const* original, Instruction* copy) {
        map.insert({ original, copy });
    }

    Instruction* get(Instruction* inst) const {
        auto itr = map.find(inst);
        VERIFY(itr != map.end(), "Cannot find cloned instruction");
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
    MaterialSourceData& sourceData;
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

    ///
    utl::hashset<Value*> allUserParametersAndTextureConstants = {};

    ///
    utl::hashset<std::string> usedValueNames = {};

    /// Type of the vector parameter buffer
    AggregateType const* primitiveBufferType = nullptr;
    AggregateType const* cameraInputType = nullptr;
    AggregateType const* vertex3DType = nullptr;
    AggregateType const* PBRDataType = nullptr;
    Type const* entityIDType = nullptr;
    AggregateType const* shadingFragmentOutType = nullptr;
    static constexpr int ReservedTextureSlots = 8;
    EnumArray<MaterialShaderType, int8_t> texBindingCounter =
        makeEnumArray<MaterialShaderType, int8_t>(ReservedTextureSlots);

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
    EnumArray<MaterialShaderType, int8_t> makeTextureBinding(
        TextureConstant const& tex);

    void defineShadingScenario();
    void analyzeShadingFunction(Function const& F, ScenarioMetadata& MD);
    Value* postprocessMaterial(FunctionBuilder& builder, Value* value,
                               ScenarioMetadata const& MD);
    Function* declarePBRShader(Type const* materialType);
    Value* callPBRShader(FunctionBuilder& builder, Value* materialData,
                         ScenarioMetadata const& scenarioMD);
    Value* transformAttribFloat3(FunctionBuilder& builder, std::string name,
                                 Value* transformMatrix, Value* attrib,
                                 VertexAttribKind kind);
    Value* transformAttribFloat4(FunctionBuilder& builder, std::string name,
                                 Value* transformMatrix, Value* attrib);
    void uniqueNames(Function& F);
    void uniqueNames(std::span<Value* const> values,
                     utl::hashset<Value*>& visited,
                     utl::hashset<std::string>& instNames);
    void uniqueName(Value& value, utl::hashset<std::string>& instNames);
    void validateTypeNames();
    std::string validateName(std::string name) const;

    /// # Utils

    Function* getUserFunction(GraphRoot kind) const;
    utl::small_vector<Function*> getUserFunctions(
        std::span<GraphRoot const> kinds) const;

    /// Emits a call instruction to \p builder and adds parameters to
    /// `builder.function()` as necessary to access required data
    FunctionCall* callFunction(std::string valueName, FunctionBuilder& builder,
                               Function* callee,
                               ScenarioMetadata const& scenarioMD,
                               MaterialShaderType shaderType);
};

} // namespace

Module matc::buildModule(Context& ctx, MaterialGraph const& G,
                         MaterialSourceData& sourceData,
                         CodegenParameters codegenParams) {
    Module mod;
    Builder B{ ctx, G, sourceData, codegenParams, mod };
    B.run();
    return mod;
}

void Builder::run() {
    verifyMaterial();
    declareTypes();
    buildUserFunctions();
    defineShadingScenario();
    validateTypeNames();
    ranges::for_each(mod.functions(), [&](auto* F) { uniqueNames(*F); });
    for (auto& [kind, function]: shaderFunctions) {
        sourceData.functionNames[kind] = function->name();
    }
}

void Builder::verifyMaterial() {
    auto* shadingValue = G.getRoot(GraphRoot::ShadingFunction);
    VERIFY(shadingValue, "Graph must have a shading function");
    VERIFY(shadingValue->type() ==
               makeShadingFunctionReturnType(ctx, codegenParams.shadingModel),
           "Invalid return type");
    switch (codegenParams.transparency) {
        using enum MaterialTransparency;
    case Opaque:
        break;
    case Masked: {
        auto* transparency = G.getRoot(GraphRoot::TransparencyFunction);
        VERIFY(transparency,
               "Masked material must have a transparency function");
        VERIFY(transparency->type() == ctx.floatType(),
               "Transparency must be a float value");
        break;
    }
    }
}

void Builder::declareTypes() {
    // clang-format off
    cameraInputType = ctx.aggregateType("SceneRenderData", {
        { ctx.matrixType(4, 4), "camera" },
        { ctx.matrixType(4, 4), "inverseCamera" },
        { ctx.float3Type(), "position" },
        { ctx.float3Type(), "front" },
        { ctx.float2Type(), "viewportSize" },
        { ctx.floatType(), "time" },
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
    PBRDataType = ctx.aggregateType("PBRData", {
        { ctx.float3Type(), "albedo" },
        { ctx.float3Type(), "emissive" },
        { ctx.floatType(), "roughness" },
        { ctx.floatType(), "metallic" },
        { ctx.floatType(), "ao" },
        { ctx.float3Type(), "normal" },
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

static std::pair<uint32_t, uint32_t> getSizeAndAlign(MaterialDataType type) {
    using enum MaterialDataType;
    switch (type) {
    case Float:
        return { 4, 4 };
    case Float2:
        return { 8, 8 };
    case Float3:
        return { 16, 16 };
    case Float4:
        return { 16, 16 };
    default:
        UNREACHABLE("Invalid data type for function");
    }
}

static size_t computeBufferLayout(
    std::span<MaterialPrimitiveParameter> params) {
    auto itr = params.begin();
    uint32_t offset = 0;
    while (itr < params.end()) {
        auto type = itr->type;
        VERIFY((int)type < (int)MaterialDataType::Texture2D,
               "This function must not be used with texture parameters");
        auto [size, align] = getSizeAndAlign(type);
        offset += (offset % align == 0 ? 0 : align - offset % align);
        do {
            itr->bufferOffset = offset;
            offset += size;
            ++itr;
        } while (itr < params.end() && itr->type == type);
    }
    return offset;
}

static MaterialDataType translateType(Type const* type) {
    // clang-format off
    return visit(*type, utl::overload {
        [](matc::TextureType const& type) {
            switch (type.subtype()) {
            case TextureSubType::Texture2D:
                return MaterialDataType::Texture2D;
            case TextureSubType::TextureCube:
                return MaterialDataType::TextureCube;
            case TextureSubType::Texture2DArray:
                UNREACHABLE("Texture2DArray cannot be used by materials");
            }
        },
        [](VectorType const& type) {
            using enum MaterialDataType;
            return (MaterialDataType)((int)Float + type.numComponents() - 1);
        },
        [](Type const& type) -> MaterialDataType {
            UNREACHABLE("Invalid parameter type: ", type.name());
        },
    }); // clang-format on
}

static MaterialTextureParameter makeTextureParameter(
    TextureConstant const* value) {
    return { .type = translateType(value->type()),
             .bindingPoint = value->bindingPoints(),
             .name = value->name(),
             .value = value->asset() };
}

static MaterialPrimitiveParameter makeVectorParameter(
    VectorConstant const* value) {
    return { .type = translateType(value->type()),
             .bufferOffset = 0,
             .name = value->name(),
             .value = value->get() };
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
    // Analyze vertex attribute usage
    for (auto [kind, F]: userFunctions) {
        auto& metadata = functionMetadata[F];
        analyzeRasterizerUsage(*F, metadata.usedVertexAttributes);
        rewriteRasterizerUsage(*F, metadata);
    }
    // Analyze user parameters
    auto vectorParameters = allUserParametersAndTextureConstants |
                            Filter<VectorConstant> | ranges::to<std::vector>;
    ranges::sort(vectorParameters, vectorTypeLess);
    sourceData.primitiveParameters = vectorParameters |
                                     transform(makeVectorParameter) |
                                     ranges::to<std::vector>;
    sourceData.parameterBufferSize =
        computeBufferLayout(sourceData.primitiveParameters);
    for (auto* tex:
         allUserParametersAndTextureConstants | Filter<TextureConstant>)
    {
        tex->setBindingPoints(makeTextureBinding(*tex));
        auto param = makeTextureParameter(tex);
        if (G.userParameters().contains(tex)) {
            sourceData.textureParameters.push_back(param);
        }
        else {
            sourceData.textureConstants.push_back(param);
        }
    }
    sourceData.cullMode = codegenParams.twoSided ? TriangleCullMode::None :
                                                   TriangleCullMode::Back;
    // Add function parameters for user parameters
    primitiveBufferType = makeParameterBufferType(ctx, vectorParameters);
    for (auto& [functionKind, F]: userFunctions) {
        computeParameters(*F, materialParamMap[functionKind], vectorParameters);
        auto& ret = cast<ReturnInst const&>(F->back());
        auto* retType = ret.returnValue()->type();
        F->computeType(ctx, retType);
    }
}

EnumArray<MaterialShaderType, int8_t> Builder::makeTextureBinding(
    TextureConstant const& tex) {
    EnumArray<MaterialShaderType, int8_t> result =
        makeEnumArray<MaterialShaderType, int8_t>(
            MaterialTextureParameter::UnusedBindingPoint);
    utl::hashset<MaterialShaderType> hasSet;
    auto set = [&](MaterialShaderType type) {
        if (hasSet.insert(type).second) {
            result[type] = texBindingCounter[type]++;
        }
    };
    for (auto* inst: tex.users() | Filter<Instruction>) {
        auto* F = inst->function();
        if (!F) {
            continue;
        }
        auto itr = ranges::find(userFunctions | values, F);
        VERIFY(itr != (userFunctions | values).end(),
               "Used parameter must have a corresponding function");
        auto functionKind = itr.base()->first;
        using enum MaterialShaderType;
        switch (functionKind) {
        case GraphRoot::VertexFunction:
            set(ShadingVertex);
            set(ShadowVertex);
            break;
        case GraphRoot::ShadingFunction:
            set(ShadingFragment);
            break;
        case GraphRoot::TransparencyFunction:
            set(ShadingFragment);
            set(ShadowFragment);
            break;
        }
    }
    return result;
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
        if (isa<TextureConstant>(value) || G.userParameters().contains(&value))
        {
            usedParams.insert(&value);
            allUserParametersAndTextureConstants.insert(&value);
        }
    });
    body.push_back(new ReturnInst(ctx, "ret", root));
    for (auto& inst: body) {
        for (auto [index, operand]: inst.operands() | enumerate) {
            if (auto* instOperand = dyncast<Instruction*>(operand)) {
                inst.setOperand(index, map.get(instOperand));
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

static Value* makeUserFunctionParameter(
    Function& F, Value* param, AggregateType const* paramBufferType,
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
            auto* value = new ExtractAggregateValue(
                matParam.name(), metadata.primitiveBufferParam, index);
            return F.push_front(value);
        },
        [&](TextureConstant& matParam) {
            auto* P = F.addParameter({ matParam.name(), matParam.type() });
            metadata.parameters[P] = &matParam;
            return P;
        },
        [&](Value const& value) -> Value* { 
            UNREACHABLE("Invalid parameter value: ",
                        magic_enum::enum_name(value.nodeType()));
        }
    });
    // clang-format on
}

void Builder::computeParameters(
    Function& F, utl::hashset<Value*> const& usedParams,
    std::span<VectorConstant* const> vectorParameters) {
    auto& metadata = functionMetadata[&F];
    for (auto* param: usedParams) {
        Value* newParam = makeUserFunctionParameter(F, param,
                                                    primitiveBufferType,
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
    for (auto* input: metadata.usedVertexAttributes) {
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

void ScenarioMetadata::collectRasterizedVertexAttributes(
    auto&& functionMetadata) {
    ranges::copy(functionMetadata |
                     transform(&FunctionMetadata::usedVertexAttributes) | join,
                 std::inserter(rasterizedVertexAttributes,
                               rasterizedVertexAttributes.begin()));
}

void ScenarioMetadata::collectComputedVertexAttributes(
    auto&& functionMetadata) {
    ranges::copy(functionMetadata |
                     transform(&FunctionMetadata::usedVertexAttributes) | join,
                 std::inserter(computedVertexAttributes,
                               computedVertexAttributes.begin()));
}

void ScenarioMetadata::createRasterizerDataType(Context& ctx,
                                                std::string typeName) {
    if (rasterizedVertexAttributes.empty()) {
        return;
    }
    auto* type =
        ctx.aggregateType(std::move(typeName), rasterizedVertexAttributes |
                                                   transform([&](auto* value) {
        return AggregateElement{ value->type(),
                                 std::string(toSpelling(value->kind())),
                                 toAttrib(value->kind()) };
    }) | ranges::to<std::vector>);
    auto map = makeEnumArray<RasterizerInputKind>(-1);
    for (auto [index, value]: rasterizedVertexAttributes | enumerate) {
        map[value->kind()] = utl::narrow_cast<int>(index);
    }
    rasterizerDataType = type;
    rasterizerDataIndexMap = map;
}

static Value* getRasterizeredVertexAttrib(std::string name,
                                          FunctionBuilder& builder,
                                          RasterizerInputKind kind,
                                          FunctionMetadata& MD,
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
    auto userFragFunctions = getUserFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    scenarioMD.collectRasterizedVertexAttributes(
        userFragFunctions |
        transform([&](auto* F) -> auto& { return functionMetadata[F]; }));
    scenarioMD.markRasterized(
        ctx.rasterizerInput(RasterizerInputKind::ClipPosition));
    if (codegenParams.shadingModel == MaterialShadingModel::DefaultLit) {
        scenarioMD.markRasterized(
            ctx.rasterizerInput(RasterizerInputKind::WorldPosition));
    }
    Function* shadingFunction = getUserFunction(GraphRoot::ShadingFunction);
    analyzeShadingFunction(*shadingFunction, scenarioMD);
    scenarioMD.createRasterizerDataType(ctx, "ShadingRasterizerData");
    fragMD.rasterizerParam = fragShader.addParameter(
        { "in", scenarioMD.rasterizerDataType, ParameterQualifier::None,
          ParameterAttribute::StageIn });
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
    auto builder = FunctionBuilder(ctx, fragShader);
    auto userVertAndFragFunctions = getUserFunctions(
        { { GraphRoot::VertexFunction, GraphRoot::ShadingFunction,
            GraphRoot::TransparencyFunction } });
    scenarioMD.collectComputedVertexAttributes(
        userVertAndFragFunctions |
        transform([&](auto* F) -> auto& { return functionMetadata[F]; }));
    if (codegenParams.transparency == MaterialTransparency::Masked) {
        auto* transparencyFunction =
            getUserFunction(GraphRoot::TransparencyFunction);
        auto* transparency = callFunction("transparency", builder,
                                          transparencyFunction, scenarioMD,
                                          MaterialShaderType::ShadingFragment);
        auto* condition =
            builder.insert<CompareInst>(ctx, "condition", CompareInst::Less,
                                        transparency, ctx.floatConstant(0.5));
        builder.insert<DiscardFragment>("discard", condition);
    }
    Value* materialData = callFunction("material", builder, shadingFunction,
                                       scenarioMD,
                                       MaterialShaderType::ShadingFragment);
    materialData = postprocessMaterial(builder, materialData, scenarioMD);
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
                               GlobalMaterialBindingType::EntityIDBuffer);
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
        { .name = "sceneDataBuffer",
          .type = cameraInputType,
          .qualifier = ParameterQualifier::DeviceConstRef,
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingVertex,
                                     GlobalMaterialBindingType::SceneData) });
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
    auto toFloat3 = [&](Value* value) {
        return builder.insert<ExtractVectorValue>(value->name(), value, 0, 1,
                                                  2);
    };
    auto* vertex = builder.insert<ArrayAccess>("V", vertexBuffer, vertexID);
    using enum RasterizerInputKind;
    vertMD.vertexAttributes[WorldPosition] =
        toFloat3(transformAttribFloat3(builder, "posWS", transformMatrix,
                                       builder.getElem("vPosition", vertex, 0),
                                       VertexAttribKind::Position));
    if (scenarioMD.isComputed(WorldNormals)) {
        vertMD.vertexAttributes[WorldNormals] = toFloat3(
            transformAttribFloat3(builder, "normalWS", transformMatrix,
                                  builder.getElem("vNormal", vertex, 1),
                                  VertexAttribKind::Direction));
    }
    if (scenarioMD.isComputed(WorldTangents)) {
        vertMD.vertexAttributes[WorldTangents] = toFloat3(
            transformAttribFloat3(builder, "tangentWS", transformMatrix,
                                  builder.getElem("vTangent", vertex, 2),
                                  VertexAttribKind::Direction));
    }
    if (scenarioMD.isComputed(WorldBinormals)) {
        vertMD.vertexAttributes[WorldBinormals] = toFloat3(
            transformAttribFloat3(builder, "binormalWS", transformMatrix,
                                  builder.getElem("vBinormal", vertex, 3),
                                  VertexAttribKind::Direction));
    }
    if (scenarioMD.isComputed(VertexColor)) {
        vertMD.vertexAttributes[VertexColor] =
            builder.getElem("vertexColor", vertex, 4);
    }
    if (scenarioMD.isComputed(TextureCoords)) {
        auto* array = builder.getElem("tmp", vertex, 5);
        vertMD.vertexAttributes[TextureCoords] =
            builder.insert<ArrayAccess>("coords", array,
                                        ctx.intConstant(IntSubType::I32, 0));
    }
    if (Function* vertexFunction = getUserFunction(GraphRoot::VertexFunction)) {
        auto* WPO = callFunction("WPO", builder, vertexFunction, scenarioMD,
                                 MaterialShaderType::ShadingVertex);
        vertMD.vertexAttributes[WorldPosition] =
            builder
                .insert<ArithmeticInst>("OffsetWorldPos", ArithmeticInst::Add,
                                        vertMD.vertexAttributes[WorldPosition],
                                        WPO);
    }
    vertMD.vertexAttributes[ClipPosition] =
        transformAttribFloat3(builder, "posCS", cameraMatrix,
                              vertMD.vertexAttributes[WorldPosition],
                              VertexAttribKind::Position);
    auto rasterizedAttribs =
        scenarioMD.rasterizedVertexAttributes |
        transform([](auto* input) { return input->kind(); }) | ToSmallVector<>;
    ranges::sort(rasterizedAttribs, ranges::less{}, [&](auto kind) {
        return scenarioMD.rasterizerDataIndexMap[kind];
    });
    auto* VSRetval =
        builder.makeAggregate("retval", scenarioMD.rasterizerDataType,
                              rasterizedAttribs | transform([&](auto kind) {
        auto* value = vertMD.vertexAttributes[kind];
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
    VERIFY(ret, "Function has no return instruction");
    if (auto* value = dyncast<AggregateBuilder const*>(ret->returnValue())) {
        auto* normals = value->element((size_t)DefaultLitMaterialValue::Normal);
        if (normals == ctx.float3Constant({ 0, 0, 1 })) {
            MD.hasCustomNormals = false;
        }
    }
    using enum RasterizerInputKind;
    MD.markRasterized(ctx.rasterizerInput(WorldNormals));
    if (MD.hasCustomNormals) {
        MD.markRasterized(ctx.rasterizerInput(WorldTangents));
        MD.markRasterized(ctx.rasterizerInput(WorldBinormals));
    }
}

static Value* getRasterizerData(FunctionBuilder& builder,
                                RasterizerInputKind kind,
                                FunctionMetadata const& functionMD,
                                ScenarioMetadata const& scenarioMD) {
    return builder.getElem(std::string(magic_enum::enum_name(kind)),
                           functionMD.rasterizerParam,
                           scenarioMD.rasterizerDataIndexMap[kind]);
}

Value* Builder::postprocessMaterial(FunctionBuilder& builder, Value* value,
                                    ScenarioMetadata const& MD) {
    switch (codegenParams.shadingModel) {
    case MaterialShadingModel::Unlit:
        return builder.clampMax("emissive", value, { 0, 0, 0, 0 });
    case MaterialShadingModel::DefaultLit: {
        using enum DefaultLitMaterialValue;
        EnumArray<DefaultLitMaterialValue, Value*> values{};
        for (auto kind: magic_enum::enum_values<DefaultLitMaterialValue>()) {
            values[kind] =
                builder.getElem(std::string(magic_enum::enum_name(kind)), value,
                                (int)kind);
        }
        values[Albedo] = builder.clampMax(values[Albedo]->name(),
                                          values[Albedo], { 0, 0, 0, 0 });
        values[Emissive] = builder.clampMax(values[Emissive]->name(),
                                            values[Emissive], { 0, 0, 0, 0 });
        auto getVertexAttrib = [&,
                                &fnMD = functionMetadata[builder.function()]](
                                   RasterizerInputKind kind) {
            return getRasterizerData(builder, kind, fnMD, MD);
        };
        auto normalize = [&](std::string name, Value* value) {
            return builder.callBuiltin(std::move(name),
                                       BuiltinFunctionKind::Normalize,
                                       { value });
        };
        if (MD.hasCustomNormals) {
            using enum RasterizerInputKind;
            // clang-format off
            auto* TBN = builder.buildMatrix( "TBN", {
                normalize("VertexTangents", getVertexAttrib(WorldTangents)),
                normalize("VertexBinormals", getVertexAttrib(WorldBinormals)),
                normalize("VertexNormals", getVertexAttrib(WorldNormals)),
            }); // clang-format on
            values[Normal] =
                builder.insert<ArithmeticInst>("normalsWS", ArithmeticInst::Mul,
                                               TBN, values[Normal]);
        }
        else {
            using enum RasterizerInputKind;
            values[Normal] = getVertexAttrib(WorldNormals);
        }
        values[Normal] = normalize("MaterialNormals", values[Normal]);
        return builder.makeAggregate("postprocessed", PBRDataType,
                                     values | ranges::to<std::vector>);
    }
    }
}

Value* Builder::callPBRShader(FunctionBuilder& builder, Value* materialData,
                              ScenarioMetadata const& scenarioMD) {
    Function* pbrShader = declarePBRShader(materialData->type());
    Function& shader = *builder.function();
    auto& MD = functionMetadata[&shader];
    auto* worldPosition =
        getRasterizeredVertexAttrib("worldPosition", builder,
                                    RasterizerInputKind::WorldPosition, MD,
                                    scenarioMD);
    auto* cameraBuffer = shader.addParameter(
        { .name = "cameraBuffer",
          .type = cameraInputType,
          .qualifier = ParameterQualifier::DeviceConstRef,
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                     GlobalMaterialBindingType::SceneData) });
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
    auto* brdfMap = shader.addParameter(
        { .name = "brdfMap",
          .type = ctx.textureType(TextureSubType::Texture2D),
          .attribute =
              getGlobalBindingAttrib(MaterialShaderType::ShadingFragment,
                                     GlobalMaterialBindingType::BRDFTexture) });
    return builder.callFunction("color", pbrShader,
                                { { materialData, worldPosition, cameraPosition,
                                    lightSpaceTransforms, lightBuffer,
                                    shadowMaps, specularMap, brdfMap } });
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
    auto values = concat(F.parameters() | transform(AddressOf<Value>),
                         F | transform(AddressOf<Value>)) |
                  ToSmallVector<>;
    utl::hashset<std::string> instNames = usedValueNames;
    utl::hashset<Value*> visited;
    uniqueNames(values, visited, instNames);
}

void Builder::uniqueNames(std::span<Value* const> values,
                          utl::hashset<Value*>& visited,
                          utl::hashset<std::string>& instNames) {
    for (auto* value: values) {
        if (!visited.insert(value).second) {
            continue;
        }
        if (auto* user = dyncast<User*>(value)) {
            uniqueNames(user->operands(), visited, instNames);
        }
        uniqueName(*value, instNames);
    }
}

void Builder::uniqueName(Value& value, utl::hashset<std::string>& instNames) {
    static std::regex const R("_\\d+$");
    value.setName(validateName(value.name()));
    while (true) {
        bool isValid = !usedValueNames.contains(value.name());
        if (isa<Instruction>(value)) {
            isValid &= !instNames.contains(value.name());
        }
        if (isValid) {
            usedValueNames.insert(value.name());
            if (isa<Instruction>(value)) {
                instNames.insert(value.name());
            }
            break;
        }
        auto name = value.name();
        std::smatch result;
        if (!std::regex_search(name, result, R)) {
            name += "_0";
            value.setName(std::move(name));
            continue;
        }
        VERIFY(result.size() >= 1, "Failed to find counter in value name");
        int counter = -1;
        auto [begin, end] = std::pair{ result[0] };
        auto convResult = std::from_chars(std::to_address(begin + 1),
                                          std::to_address(end), counter);
        VERIFY(convResult.ec == std::errc{}, "Failed to parse counter value");
        VERIFY(counter >= 0, "");
        name.replace(begin + 1, end, std::to_string(counter + 1));
        value.setName(std::move(name));
    }
}

void Builder::validateTypeNames() {
    auto allTypes = ctx.types();
    for (auto* type: allTypes | Filter<AggregateType>) {
        auto* mutType = const_cast<AggregateType*>(type);
        mutType->setName(validateName(type->name()));
        for (auto [index, elem]: type->elements() | enumerate) {
            mutType->setElementName(index, validateName(elem.name));
        }
    }
}

std::string Builder::validateName(std::string name) const {
    static utl::hashset<std::string> const keywords{
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "atomic_cancel",
        "atomic_commit",
        "atomic_noexcept",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "concept",
        "const",
        "consteval",
        "constexpr",
        "constinit",
        "const_cast",
        "continue",
        "co_await",
        "co_return",
        "co_yield",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "reflexpr",
        "register",
        "reinterpret_cast",
        "requires",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "static_assert",
        "static_cast",
        "struct",
        "switch",
        "synchronized",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
        "device",
        "vertex",
        "fragment",
        "kernel",
        "float1",
        "float2",
        "float3",
        "float4",
        "float1x1",
        "float2x1",
        "float3x1",
        "float4x1",
        "float1x2",
        "float2x2",
        "float3x2",
        "float4x2",
        "float1x3",
        "float2x3",
        "float3x3",
        "float4x3",
        "float1x4",
        "float2x4",
        "float3x4",
        "float4x4",
    };
    if (keywords.contains(name)) {
        name += '_';
    }
    ranges::replace_if(name, [](char value) {
        return !std::isalnum(value) && value != '_';
    }, '_');
    return name;
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
    using enum MaterialShaderType;
    switch (AC.shaderType) {
    case ShadingVertex:
        [[fallthrough]];
    case ShadowVertex:
        return AC.callerMD.vertexAttributes[input->kind()];
    case ShadingFragment:
        [[fallthrough]];
    case ShadowFragment:
        return getRasterizeredVertexAttrib(std::move(name), AC.builder, kind,
                                           AC.callerMD, AC.scenarioMD);
    }
}

static Value* getFunctionArgument(TextureConstant* texParam,
                                  ArgumentContext AC) {
    auto itr = AC.callerMD.texParamMap.find(texParam);
    if (itr != AC.callerMD.texParamMap.end()) {
        return itr->second;
    }
    ParameterAttribute attrib = { ParameterAttribute::Texture,
                                  texParam->bindingPoint(AC.shaderType) };
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
                                    ScenarioMetadata const& scenarioMD,
                                    MaterialShaderType shaderType) {
    auto* caller = builder.function();
    auto& callerMD = functionMetadata[caller];
    auto& calleeMD = functionMetadata[callee];
    auto arguments = callee->parameters() |
                     transform([&](FunctionParameter const& param) {
        auto itr = calleeMD.parameters.find(&param);
        VERIFY(itr != calleeMD.parameters.end(), "Failed to find parameter");
        return std::visit([&](auto paramMetadata) {
            ArgumentContext args = {
                .builder = builder,
                .scenarioMD = scenarioMD,
                .callerMD = callerMD,
                .calleeMD = calleeMD,
                .shaderType = shaderType,
                .primitiveBufferType = primitiveBufferType,
            };
            return getFunctionArgument(paramMetadata, args);
        }, itr->second);
    }) | ToSmallVector<>;
    return builder.callFunction(std::move(valueName), callee, arguments);
}

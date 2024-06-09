#include "Bloom/MaterialCompiler/MaterialGraph.h"

#include <cassert>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hashmap.hpp>
#include <utl/overload.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/Graphics/Material/MaterialSourceData.h"
#include "Bloom/MaterialCompiler/CodeGen.h"
#include "Bloom/MaterialCompiler/FunctionBuilder.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

[[noreturn]] static void unreachable() { __builtin_unreachable(); }

Value* MaterialGraph::addValue(DynUniquePtr<Value> value) {
    _values.push_back(std::move(value));
    return _values.back().get();
}

Value* MaterialGraph::addVectorParameter(Context& ctx, std::string name,
                                         int numComponents,
                                         vml::float4 defaultValue) {
    auto value = allocate<VectorConstant>(ctx, std::move(name), numComponents,
                                          defaultValue);
    _params.insert(value.get());
    return addValue(std::move(value));
}

TextureConstant* MaterialGraph::addTextureParameter(Context& ctx,
                                                    std::string name,
                                                    TextureSubType subtype,
                                                    AssetHandle asset) {
    auto value = allocate<TextureConstant>(std::move(name),
                                           ctx.textureType(subtype), asset);
    _params.insert(value.get());
    return addValue(std::move(value));
}

utl::small_vector<Function*> MaterialGraph::functions() const {
    auto getValues = [](auto&& rng) {
        return rng | ranges::views::values | transform(ToAddress<>);
    };
    return concat(getValues(_functions), getValues(_shaders)) | ToSmallVector<>;
}

Function* MaterialGraph::getFunction(GraphRoot kind) const {
    auto itr = _functions.find(kind);
    return itr != _functions.end() ? itr->second.get() : nullptr;
}

utl::small_vector<Function*> MaterialGraph::getFunctions(
    std::span<GraphRoot const> kinds) const {
    utl::small_vector<Function*> result;
    for (auto kind: kinds) {
        if (auto* F = getFunction(kind)) {
            result.push_back(F);
        }
    }
    return result;
}

static MaterialDataType translate(matc::TextureType const* type) {
    switch (type->subtype()) {
    case TextureSubType::Texture2D:
        return MaterialDataType::Texture2D;
    case TextureSubType::TextureCube:
        return MaterialDataType::TextureCube;
    }
}

static MaterialDataType translate(VectorType const* type) {
    using enum MaterialDataType;
    return (MaterialDataType)((int)Float + type->numComponents() - 1);
}

static Type const* translate(Context& ctx, MaterialDataType type) {
    switch (type) {
    case MaterialDataType::Float:
        return ctx.floatType();
    case MaterialDataType::Float2:
        return ctx.float2Type();
    case MaterialDataType::Float3:
        return ctx.float3Type();
    case MaterialDataType::Float4:
        return ctx.float4Type();
    case MaterialDataType::Texture2D:
        return ctx.textureType(TextureSubType::Texture2D);
    case MaterialDataType::TextureCube:
        return ctx.textureType(TextureSubType::TextureCube);
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

static std::vector<MaterialPrimitiveParameter> makePrimitiveParameters(
    std::span<VectorConstant const* const> params) {
    return params | transform([](VectorConstant const* param) {
        return MaterialPrimitiveParameter{ .type = translate(param->type()),
                                           .bufferOffset = 0,
                                           .name = param->name(),
                                           .value = param->get() };
    }) | ranges::to<std::vector>;
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

static RasterizerMetadata makeRasterizerMetadata(
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
        unreachable();
    }
}

static size_t computeBufferLayout(
    std::span<MaterialPrimitiveParameter> params) {
    auto itr = params.begin();
    uint32_t offset = 0;
    while (itr < params.end()) {
        auto type = itr->type;
        assert((int)type < (int)MaterialDataType::Texture2D &&
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

static std::string functionName(GraphRoot kind) {
    return std::string(magic_enum::enum_name(kind));
}

static bool vectorTypeLess(VectorConstant const* a, VectorConstant const* b) {
    return a->type()->numComponents() < b->type()->numComponents();
}

MaterialSourceData MaterialGraph::generate(Context& ctx) {
    MaterialSourceData sourceData;
    buildFunctions(ctx, sourceData);
    auto generator = makeCodeGenerator(ctx, *this);
    sourceData.source = generator->run();
    return sourceData;
}

namespace {

enum class ShaderScenario { Shading, Shadow };

} // namespace

static void analyzeRasterizerUsageOfFunction(
    Function& function, utl::hashset<RasterizerInput*>& usedInputs) {
    for (auto& inst: function) {
        for (auto* input: inst.operands() | Filter<RasterizerInput>) {
            usedInputs.insert(input);
        }
    }
}

static void rewriteRasterizerUsage(Function& F, FunctionMetadata& metadata) {
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
                param = F.addParameter(std::move(name), input->type());
                metadata.parameters[param] = input;
            }
            inst->replaceOperand(input, param);
        }
    }
}

void MaterialGraph::buildFunctions(Context& ctx,
                                   MaterialSourceData& sourceData) {
    utl::hashset<Value*> activeValues;
    utl::hashmap<GraphRoot, utl::hashset<Value*>> materialParamMap;
    // Build functions
    for (auto [functionKind, root]: _roots) {
        _functions[functionKind] =
            buildFunction(ctx, root, functionName(functionKind),
                          materialParamMap[functionKind]);
    }
    // Analyze rasterizer usage
    auto fragmentFunctions = getFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    for (auto* F: fragmentFunctions) {
        auto& metadata = functionMetadata[F];
        analyzeRasterizerUsageOfFunction(*F, metadata.usedRasterizerInputs);
        rewriteRasterizerUsage(*F, metadata);
    }
    auto vectorParameters = materialParamMap | ranges::views::values | join |
                            Filter<VectorConstant> | ranges::to<std::vector>;
    ranges::sort(vectorParameters, vectorTypeLess);
    primitiveBufferType = makeParameterBufferType(ctx, vectorParameters);
    for (auto& [functionKind, F]: _functions) {
        computeParameters(ctx, *F, primitiveBufferType,
                          materialParamMap[functionKind], vectorParameters);
        auto& ret = cast<ReturnInst const&>(F->back());
        auto* retType = ret.returnValue()->type();
        F->computeType(ctx, retType);
    }
    static constexpr int ReservedTextureSlots = 8;
    int textureBindingPoint = ReservedTextureSlots;
    for (auto* tex: ctx.textureConstants()) {
        if (tex->users().empty()) {
            continue;
        }
        int bindingPoint = textureBindingPoint++;
        tex->setBindingPoint(bindingPoint);
    }
    defineShadingScenario(ctx);
}

namespace {

struct ArgumentContext {
    FunctionBuilder& builder;
    RasterizerMetadata const& rasterizerMetadata;
    FunctionMetadata& callerMD;
    FunctionMetadata& calleeMD;
    MaterialShaderType shaderType;
    Type const* primitiveBufferType;
};

} // namespace

static Value* getFunctionArgument(RasterizerInput* input, ArgumentContext AC) {
    auto kind = input->kind();
    std::string name(magic_enum::enum_name(kind));
    return AC.builder
        .insert<ExtractAggregateValue>(std::move(name),
                                       AC.callerMD.rasterizerParam,
                                       AC.rasterizerMetadata.indexMap[kind]);
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
    auto* param = caller->addParameter(texParam->name(), texParam->type(),
                                       ParameterQualifier::None, attrib);
    AC.callerMD.texParamMap.insert({ texParam, param });
    return param;
}

static Value* getFunctionArgument(ParameterBufferTag, ArgumentContext AC) {
    auto& param = AC.callerMD.primitiveBufferParam;
    if (param) {
        return param;
    }
    ParameterAttribute attrib{
        ParameterAttribute::Buffer,
        getGlobalMaterialBindingPoint(
            AC.shaderType, GlobalMaterialBindingType::MaterialParameterBuffer)
            .index
    };
    auto* caller = AC.builder.function();
    param = caller->addParameter("primitives", AC.primitiveBufferType,
                                 ParameterQualifier::DeviceConstRef, attrib);
    return param;
}

// MaterialShaderType::ShadingFragment

/// Emits a call instruction to \p builder and adds parameters to
/// `builder.function()` as necessary to access required data
FunctionCall* MaterialGraph::callFunction(
    FunctionBuilder& builder, Function* callee,
    RasterizerMetadata const& rasterizerMetadata) {
    auto* caller = builder.function();
    auto& callerMD = functionMetadata[caller];
    auto& calleeMD = functionMetadata[callee];
    auto arguments = callee->parameters() |
                     transform([&](FunctionParameter const& param) {
        auto itr = calleeMD.parameters.find(&param);
        assert(itr != calleeMD.parameters.end());
        return std::visit([&](auto X) {
            return getFunctionArgument(
                X, {
                       .builder = builder,
                       .rasterizerMetadata = rasterizerMetadata,
                       .callerMD = callerMD,
                       .calleeMD = calleeMD,
                       .shaderType = MaterialShaderType::ShadingFragment,
                       .primitiveBufferType = primitiveBufferType,
                   });
        }, itr->second);
    }) | ToSmallVector<>;
    return builder.callFunction("transparency", callee, arguments);
}

void MaterialGraph::defineShadingScenario(Context& ctx) {
    // Fragment Shader
    _shaders[MaterialShaderType::ShadingFragment] =
        allocate<Function>("FragmentShading", nullptr, ShaderKind::Fragment);
    auto& fragShader = *_shaders[MaterialShaderType::ShadingFragment];
    auto& fragMD = functionMetadata[&fragShader];
    auto userFunctions = getFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    auto usedRasterizerData = userFunctions | transform([&](auto* F) -> auto& {
        auto& MD = functionMetadata[F];
        return MD.usedRasterizerInputs;
    }) | join | ToSmallVector<>;
    auto rasterizerMetadata = makeRasterizerMetadata(ctx,
                                                     "ShadingRasterizerData",
                                                     usedRasterizerData);
    fragMD.rasterizerParam =
        fragShader.addParameter("in", rasterizerMetadata.type,
                                ParameterQualifier::None,
                                ParameterAttribute::StageIn);
    utl::hashmap<TextureConstant*, FunctionParameter*> texParamMap;
    auto builder = FunctionBuilder(ctx, fragShader);
    if (codegenParameters().transparency == MaterialTransparency::Masked) {
        auto* transparencyFunction =
            getFunction(GraphRoot::TransparencyFunction);
        assert(transparencyFunction);
        callFunction(builder, transparencyFunction, rasterizerMetadata);
        //        builder.insert<DiscardFragment>("discard", /*condition: */
        //        nullptr);
    }
    fragShader.computeType(ctx, ctx.voidType());
    // Vertex Shader
}

namespace {

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

} // namespace

DynUniquePtr<Function> MaterialGraph::buildFunction(
    Context& ctx, Value* root, std::string name,
    utl::hashset<Value*>& usedParams) {
    CloneMap map;
    List<Instruction> body;
    traverseTopsort(root, [&](auto& value) {
        if (auto* inst = dyncast<Instruction const*>(&value)) {
            body.push_back(inst->clone().release());
            auto* clone = &body.back();
            map.insert(inst, clone);
        }
    });
    for (auto& inst: body) {
        for (auto [index, operand]: inst.operands() | enumerate) {
            if (auto* instOperand = dyncast<Instruction*>(operand)) {
                inst.setOperand(index, map.get(instOperand));
            }
            if (_params.contains(operand)) {
                usedParams.insert(operand);
            }
        }
    }
    body.push_back(new ReturnInst(ctx, "ret", map.get(root)));
    return allocate<Function>(std::move(name), nullptr, ShaderKind::None,
                              List<FunctionParameter>{}, std::move(body));
}

static Value* makeParameter(Function& F, Value* param,
                            AggregateType const* paramBufferType,
                            FunctionParameter*& primitiveBuffer,
                            std::span<VectorConstant* const> vectorParameters,
                            FunctionMetadata& metadata) {
    // clang-format off
    return visit(*param, utl::overload{
        [&](VectorConstant const& matParam) {
            if (!primitiveBuffer) {
                auto* P = F.addParameter(
                    "primitives", paramBufferType,
                    ParameterQualifier::DeviceConstRef);
                metadata.parameters[P] = ParameterBufferTag{};
                primitiveBuffer = P;
            }
            auto itr = ranges::find(vectorParameters, &matParam);
            int index = utl::narrow_cast<int>(itr - vectorParameters.begin());
            auto* value = new ExtractAggregateValue(matParam.name(),
                                                    primitiveBuffer, index);
            return F.push_front(value);
        },
        [&](TextureConstant& matParam) {
            auto* P = F.addParameter(
                new FunctionParameter(matParam.name(), matParam.type()));
            metadata.parameters[P] = &matParam;
            return P;
        },
        [&](Value const&) -> Value* { unreachable(); }
    });
    // clang-format on
}

void MaterialGraph::computeParameters(
    Context& ctx, Function& F, AggregateType const* paramBufferType,
    utl::hashset<Value*> const& usedParams,
    std::span<VectorConstant* const> vectorParameters) {
    FunctionParameter* primitiveBuffer = nullptr;
    auto& metadata = functionMetadata[&F];
    for (auto* param: usedParams) {
        Value* newParam = makeParameter(F, param, paramBufferType,
                                        primitiveBuffer, vectorParameters,
                                        metadata);
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

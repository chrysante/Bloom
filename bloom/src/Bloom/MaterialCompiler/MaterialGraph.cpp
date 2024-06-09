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

namespace {

struct FunctionMetadata {
    utl::hashset<RasterizerInput*> usedRasterizerInputs;
};

struct RasterizerMetadata {
    AggregateType const* type;
    EnumArray<RasterizerInputKind, int> indexMap;
};

} // namespace

static RasterizerMetadata makeRasterizerDataType(
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

static void rewriteRasterizerUsage(
    utl::hashset<RasterizerInput*> const& usedRasterizerInputs) {
    utl::hashmap<Function const*,
                 EnumArray<RasterizerInputKind, FunctionParameter*>>
        paramMap;
    for (auto* input: usedRasterizerInputs) {
        for (auto* inst: input->users() | Filter<Instruction> | ToSmallVector<>)
        {
            auto* F = inst->function();
            if (!F) {
                continue;
            }
            auto& param = paramMap[F][input->kind()];
            if (!param) {
                std::string name(magic_enum::enum_name(input->kind()));
                param = F->addParameter(std::move(name), input->type());
            }
            inst->replaceOperand(input, param);
        }
    }
}

void MaterialGraph::buildFunctions(Context& ctx,
                                   MaterialSourceData& sourceData) {
    utl::hashset<Value*> activeValues;
    utl::hashmap<GraphRoot, utl::hashset<Value*>> paramMap;
    // Build functions
    for (auto [functionKind, root]: _roots) {
        _functions[functionKind] = buildFunction(ctx, root,
                                                 functionName(functionKind),
                                                 paramMap[functionKind]);
    }
    utl::hashmap<Function*, FunctionMetadata> functionMetadata;
    // Analyze rasterizer usage
    auto fragmentFunctions = getFunctions(
        { { GraphRoot::ShadingFunction, GraphRoot::TransparencyFunction } });
    for (auto* F: fragmentFunctions) {
        auto& metadata = functionMetadata[F];
        analyzeRasterizerUsageOfFunction(*F, metadata.usedRasterizerInputs);
        rewriteRasterizerUsage(metadata.usedRasterizerInputs);
    }
    auto vectorParameters = paramMap | ranges::views::values | join |
                            Filter<VectorConstant> | ranges::to<std::vector>;
    ranges::sort(vectorParameters, vectorTypeLess);
    auto* paramBufferType = makeParameterBufferType(ctx, vectorParameters);
    for (auto& [functionKind, F]: _functions) {
        computeParameters(ctx, *F, paramBufferType, paramMap[functionKind],
                          vectorParameters);
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
    return allocate<Function>(std::move(name), nullptr,
                              List<FunctionParameter>{}, std::move(body));
}

static Value* makeParameter(
    Function& F, Value* param, AggregateType const* paramBufferType,
    FunctionParameter*& primitiveBuffer,
    std::span<VectorConstant const* const> vectorParameters) {
    // clang-format off
    return visit(*param, utl::overload{
        [&](VectorConstant const& param) {
            if (!primitiveBuffer) {
                primitiveBuffer = F.addParameter(
                    "primitives", paramBufferType,
                    ParameterQualifier::DeviceConstRef);
            }
            auto itr = ranges::find(vectorParameters, &param);
            int index = utl::narrow_cast<int>(itr - vectorParameters.begin());
            auto* value = new ExtractAggregateValue(param.name(), 
                                                    primitiveBuffer, index);
            return F.push_front(value);
        },
        [&](TextureConstant const& param) {
            return F.addParameter(
                new FunctionParameter(param.name(), param.type()));
        },
        [&](Value const&) -> Value* { unreachable(); }
    });
    // clang-format on
}

void MaterialGraph::computeParameters(
    Context& ctx, Function& F, AggregateType const* paramBufferType,
    utl::hashset<Value*> const& usedParams,
    std::span<VectorConstant const* const> vectorParameters) {
    FunctionParameter* primitiveBuffer = nullptr;
    for (auto* param: usedParams) {
        Value* newParam = makeParameter(F, param, paramBufferType,
                                        primitiveBuffer, vectorParameters);
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

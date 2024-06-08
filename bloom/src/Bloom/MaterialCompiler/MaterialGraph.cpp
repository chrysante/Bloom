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

static AggregateType const* makeParameterBufferType(
    Context& ctx, std::span<VectorConstant const* const> params) {
    if (params.empty()) {
        return nullptr;
    }
    return ctx.aggregateType("PrimitiveParameters",
                             params | transform([&](auto& param) mutable {
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

static int getParamBufferIndex(GraphRoot kind) {
    using enum GraphRoot;
    using enum MaterialShaderType;
    auto bindingType = GlobalMaterialBindingType::MaterialParameterBuffer;
    switch (kind) {
    case VertexFunction:
        return getGlobalMaterialBindingPoint(Vertex, bindingType).index;
    case LitFragmentFunction:
        return getGlobalMaterialBindingPoint(Fragment, bindingType).index;
    case UnlitFragmentFunction:
        return getGlobalMaterialBindingPoint(Fragment, bindingType).index;
    case TransparencyFunction:
        return getGlobalMaterialBindingPoint(Fragment, bindingType).index;
    }
}

static std::string functionName(GraphRoot kind) {
    return std::string(magic_enum::enum_name(kind));
}

static bool vectorTypeLess(VectorConstant const* a, VectorConstant const* b) {
    return a->type()->numComponents() < b->type()->numComponents();
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

void MaterialGraph::computeParameters(
    Context& ctx, Function& F, AggregateType const* paramBufferType,
    utl::hashset<Value*> const& usedParams,
    std::span<VectorConstant const* const> vectorParameters) {
    List<FunctionParameter> params;
    FunctionParameter* primitiveBuffer = nullptr;
    for (auto* param: usedParams) {
        auto* newParam =
            visit(*param, utl::overload{ [&](VectorConstant const& param) {
            if (!primitiveBuffer) {
                params.push_back(
                    new FunctionParameter("primitive", paramBufferType,
                                          ParameterQualifier::DeviceConstRef,
                                          { ParameterAttribute::Buffer, -1 }));
                primitiveBuffer = &params.back();
            }
            auto itr = ranges::find(vectorParameters, &param);
            int index = utl::narrow_cast<int>(itr - vectorParameters.begin());
            auto* value =
                F.push_front(new ExtractAggregateValue(param.name(),
                                                       primitiveBuffer, index));
            return value;
        }, [&](TextureConstant const& param) {
            params.push_back(
                new FunctionParameter(param.name(), param.type(),
                                      ParameterQualifier::None,
                                      { ParameterAttribute::Texture, -1 }));
            return &params.back();
        }, [&](Value const&) -> Value* { unreachable(); } });
        auto paramUsers = param->users() | ToSmallVector<>;
        for (auto* user: paramUsers) {
            if (auto* inst = dyncast<Instruction*>(user);
                inst && inst->function() == &F)
            {
                inst->replaceOperand(param, newParam);
            }
        }
    }
    F.setParameters(std::move(params));
    F.computeType(ctx, cast<ReturnInst const&>(F.back()).returnValue()->type());
}

void MaterialGraph::buildFunctions(Context& ctx) {
    utl::hashset<Value*> activeValues;
    utl::hashmap<GraphRoot, utl::hashset<Value*>> paramMap;
    for (auto functionKind: magic_enum::enum_values<GraphRoot>()) {
        auto* root = getRoot(functionKind);
        if (!root) {
            continue;
        }
        _functions[functionKind] = buildFunction(ctx, root,
                                                 functionName(functionKind),
                                                 paramMap[functionKind]);
    }
    auto vectorParameters = paramMap | ranges::views::values | join |
                            Filter<VectorConstant> | ranges::to<std::vector>;
    ranges::sort(vectorParameters, vectorTypeLess);
    auto* paramBufferType = makeParameterBufferType(ctx, vectorParameters);
    for (auto functionKind: magic_enum::enum_values<GraphRoot>()) {
        auto* F = getFunction(functionKind);
        if (!F) {
            continue;
        }
        computeParameters(ctx, *F, paramBufferType, paramMap[functionKind],
                          vectorParameters);
    }

    //    auto allVectorParameters = activeValues | Filter<VectorParameter> |
    //                               ToSmallVector<>;
    //    ranges::sort(allVectorParameters, vectorTypeLess);
    //    auto* paramBufferType = makeParameterBufferType(ctx,
    //    allVectorParameters); assert(
    //        getParamBufferIndex(GraphRoot::VertexFunction) ==
    //            getParamBufferIndex(GraphRoot::LitFragmentFunction) &&
    //        "Parameter buffer must have the same binding point in vertex and
    //        fragment functions for now");
    //    auto* paramBuffer =
    //        addValue<BufferParameter>("parameters", paramBufferType,
    //                                  getParamBufferIndex(
    //                                      GraphRoot::VertexFunction));
    //    ranges::for_each(allVectorParameters, [&, index = 0](auto* param)
    //    mutable {
    //        param->setBufferAndElemIndex(paramBuffer, index++);
    //    });
    //    static constexpr int NumReservedTextureSlots = 8;
    //    ranges::for_each(activeValues | Filter<TextureValue>,
    //                     [index = NumReservedTextureSlots](auto* tex) mutable
    //                     {
    //        tex->setBindingPoint(index++);
    //    });
    //    for (auto& [kind, values]: functions) {
    //        bool needParamBuffer = ranges::any_of(values,
    //        isa<VectorParameter>); std::vector<FunctionParameter> params; if
    //        (needParamBuffer) {
    //            params.push_back({ paramBuffer,
    //                               FunctionParameter::Qualifier::None,
    //                               { ParameterAttribute::Buffer,
    //                                 paramBuffer->bindingPoint() } });
    //        }
    //        for (auto* texParam: values | Filter<TextureParameter>) {
    //            params.push_back(
    //                { texParam,
    //                  FunctionParameter::Qualifier::None,
    //                  { ParameterAttribute::Texture, texParam->bindingPoint()
    //                  } });
    //        }
    //        _functions[kind] = std::make_unique<Function>(functionName(kind),
    //                                                      std::move(params),
    //                                                      std::move(values));
    //    }
}

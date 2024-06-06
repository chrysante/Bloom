#include "Bloom/MaterialCompiler/MaterialGraph.h"

#include <cassert>

#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/hashmap.hpp>
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

static std::vector<Value*> topsort(Value* root) {
    std::vector<Value*> sorted;
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
        sorted.push_back(value);
    };
    visit(visit, root);
    return sorted;
}

static std::vector<MaterialPrimitiveParameter> makePrimitiveParameters(
    std::span<VectorParameter const* const> params) {
    return params | transform([](VectorParameter const* param) {
        return MaterialPrimitiveParameter{ .type = translate(param->type()),
                                           .bufferOffset = 0,
                                           .name = param->name(),
                                           .value = param->get() };
    }) | ranges::to<std::vector>;
}

static AggregateType const* makeParameterBufferType(
    Context& ctx, std::span<VectorParameter const* const> params) {
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

static size_t computeBufferLayout(std::span<MaterialPrimitiveParameter> params,
                                  std::span<VectorParameter*> paramValues) {
    auto cmp = [](auto const& a, auto const& b) {
        return (int)a.first.type < (int)b.first.type;
    };
    ranges::sort(zip(params, paramValues), cmp);
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

static bool vectorTypeLess(VectorParameter const* a, VectorParameter const* b) {
    return a->type()->numComponents() < b->type()->numComponents();
}

void MaterialGraph::buildFunctions(Context& ctx) {
    utl::hashset<Value*> activeValues;
    utl::hashmap<GraphRoot, std::vector<Value*>> functions;
    for (auto kind: magic_enum::enum_values<GraphRoot>()) {
        auto* root = getRoot(kind);
        if (!root) {
            continue;
        }
        auto values = topsort(root);
        ranges::copy(values, std::inserter(activeValues, activeValues.begin()));
        functions.insert({ kind, std::move(values) });
    }
    auto newEnd = ranges::remove_if(_values, [&](auto& value) {
        return !activeValues.contains(value.get());
    });
    _values.erase(newEnd, _values.end());
    auto allVectorParameters = activeValues | Filter<VectorParameter> |
                               ToSmallVector<>;
    ranges::sort(allVectorParameters, vectorTypeLess);
    auto* paramBufferType = makeParameterBufferType(ctx, allVectorParameters);
    assert(
        getParamBufferIndex(GraphRoot::VertexFunction) ==
            getParamBufferIndex(GraphRoot::LitFragmentFunction) &&
        "Parameter buffer must have the same binding point in vertex and fragment functions for now");
    auto* paramBuffer =
        addValue<BufferParameter>("parameters", paramBufferType,
                                  getParamBufferIndex(
                                      GraphRoot::VertexFunction));
    ranges::for_each(allVectorParameters, [&, index = 0](auto* param) mutable {
        param->setBufferAndElemIndex(paramBuffer, index++);
    });
    static constexpr int NumReservedTextureSlots = 8;
    ranges::for_each(activeValues | Filter<TextureValue>,
                     [index = NumReservedTextureSlots](auto* tex) mutable {
        tex->setBindingPoint(index++);
    });
    for (auto& [kind, values]: functions) {
        bool needParamBuffer = ranges::any_of(values, isa<VectorParameter>);
        std::vector<FunctionParameter> params;
        if (needParamBuffer) {
            params.push_back({ paramBuffer,
                               FunctionParameter::Qualifier::None,
                               { ParameterAttribute::Buffer,
                                 paramBuffer->bindingPoint() } });
        }
        for (auto* texParam: values | Filter<TextureParameter>) {
            params.push_back(
                { texParam,
                  FunctionParameter::Qualifier::None,
                  { ParameterAttribute::Texture, texParam->bindingPoint() } });
        }
        _functions[kind] = std::make_unique<Function>(functionName(kind),
                                                      std::move(params),
                                                      std::move(values));
    }
}

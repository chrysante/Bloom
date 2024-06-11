#include "Bloom/MaterialCompiler/Fwd.h"

#include <vector>

#include <magic_enum.hpp>
#include <range/v3/view.hpp>

#include "Bloom/MaterialCompiler/Context.h"
#include "Bloom/MaterialCompiler/Types.h"

using namespace bloom;
using namespace matc;
using namespace ranges::views;

Type const* matc::toConcreteType(Context& ctx, SymbolicType symType) {
    using enum SymbolicType;
    switch (symType) {
    case Float:
        return ctx.floatType();
    case Float2:
        return ctx.float2Type();
    case Float3:
        return ctx.float3Type();
    case Float4:
        return ctx.float4Type();
    case FloatN:
        return nullptr;
    case VectorN:
        return nullptr;
    }
}

Type const* matc::getType(Context& ctx, DefaultLitMaterialValue value) {
    using enum DefaultLitMaterialValue;
    using enum SymbolicType;
    switch (value) {
#define BL_MATGRAPH_DEFLITVALUE(Name, Type)                                    \
    case Name:                                                                 \
        return toConcreteType(ctx, Type);
#include "Bloom/MaterialCompiler/Lists.def"
    }
}

Type const* matc::makeShadingFunctionReturnType(Context& ctx,
                                                MaterialShadingModel model) {
    using enum MaterialShadingModel;
    switch (model) {
    case Unlit:
        return ctx.float3Type();
    case DefaultLit:
        auto elements = magic_enum::enum_values<DefaultLitMaterialValue>() |
                        transform([&](auto value) {
            return AggregateElement{ .name = std::string(
                                         magic_enum::enum_name(value)),
                                     .type = getType(ctx, value) };
        }) | ranges::to<std::vector>;
        return ctx.aggregateType("DefaultLitMaterial", std::move(elements));
    }
}

std::string_view matc::toSpelling(RasterizerInputKind kind) {
    using enum RasterizerInputKind;
    switch (kind) {
    case ClipPosition:
        return "positionCS";
    case TextureCoords:
        return "UVs";
    case WorldNormals:
        return "normalWS";
    case WorldTangents:
        return "tangentWS";
    case WorldBinormals:
        return "binormalWS";
    case WorldPosition:
        return "positionWS";
    case VertexColor:
        return "vertexColor";
    }
}

std::string_view matc::toSpelling(BuiltinFunctionKind function) {
    switch (function) {
#define BL_MATGRAPH_BUILTIN(Name, Spelling, ...)                               \
    case BuiltinFunctionKind::Name:                                            \
        return #Spelling;
#include "Bloom/MaterialCompiler/Lists.def"
    }
}

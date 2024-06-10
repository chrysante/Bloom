#include "Bloom/MaterialCompiler/Context.h"

#include <range/v3/algorithm.hpp>
#include <utl/hash.hpp>
#include <utl/strcat.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/EnumCount.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Core/Utils.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace ranges::views;
using namespace matc;
using namespace vml::short_types;

using Impl = Context::Impl;

namespace {

struct FuncTypeKey {
    Type const* ret;
    utl::small_vector<Type const*> args;

    bool operator==(FuncTypeKey const&) const = default;
};

struct BuiltinKey {
    BuiltinFunctionKind kind;
    utl::small_vector<Type const*> args;

    bool operator==(BuiltinKey const&) const = default;
};

} // namespace

template <>
struct std::hash<FuncTypeKey> {
    size_t operator()(FuncTypeKey const& key) const {
        size_t retHash = utl::hash<Type const*>{}(key.ret);
        size_t argHash =
            utl::hash_combine_range(key.args.begin(), key.args.end());
        return utl::hash_combine(retHash, argHash);
    }
};

template <>
struct std::hash<BuiltinKey> {
    size_t operator()(BuiltinKey const& key) const {
        size_t kindHash = utl::hash<BuiltinFunctionKind>{}(key.kind);
        size_t argHash =
            utl::hash_combine_range(key.args.begin(), key.args.end());
        return utl::hash_combine(kindHash, argHash);
    }
};

struct Context::Impl {
    Impl();

    template <typename T, typename... Args>
    T const* addType(Args&&... args) {
        typeOwners.push_back(
            DynUniquePtr<T>(new T(std::forward<Args>(args)...)));
        return cast<T const*>(typeOwners.back().get());
    }

    template <typename T, typename... Args>
    T* addValue(Args&&... args) {
        valueOwners.push_back(
            DynUniquePtr<T>(new T(std::forward<Args>(args)...)));
        return cast<T*>(valueOwners.back().get());
    }

    template <typename KeyType, typename V, typename H, typename E>
    auto getOrInsert(utl::hashmap<KeyType, V, H, E>& map, KeyType key,
                     auto factory) {
        auto itr = map.find(key);
        if (itr != map.end()) {
            return itr->second;
        }
        auto result = factory();
        map.insert({ key, result });
        return result;
    }

    // Types
    std::vector<DynUniquePtr<Type>> typeOwners;
    VoidType const* voidType = nullptr;
    BoolType const* boolType = nullptr;
    EnumArray<IntSubType, IntType const*> intTypes;
    VectorType const* vectorType[4]{};
    utl::hashmap<std::pair<int, VectorTypeFlags>, VectorType const*>
        vectorTypes;
    utl::hashmap<std::pair<int, int>, MatrixType const*> matrixTypes;
    utl::hashmap<TextureSubType, TextureType const*> textureTypes;
    utl::hashmap<std::string, AggregateType const*> aggTypes;
    utl::hashmap<FuncTypeKey, FunctionType const*> funcTypes;
    utl::hashmap<std::pair<Type const*, Mutability>, PointerType const*>
        pointerTypes;
    utl::hashmap<std::pair<Type const*, IntConstant const*>, ArrayType const*>
        arrayTypes;
    TextureType const* textureType = nullptr;
    // Values
    std::vector<DynUniquePtr<Value>> valueOwners;
    utl::hashmap<std::pair<IntSubType, long long>, IntConstant*> intConstants;
    utl::hashmap<std::pair<int, float4>, VectorConstant*> vectorConstants;
    utl::hashmap<RasterizerInputKind, RasterizerInput*> rasterizerInputs;
    utl::hashmap<utl::uuid, TextureConstant*> textureConstants;
    utl::hashmap<BuiltinKey, BuiltinFunction*> builtins;
};

Context::Context(): impl(std::make_unique<Impl>()) {}

Context::Context(Context&&) noexcept = default;

Context& Context::operator=(Context&&) noexcept = default;

Context::~Context() = default;

Impl::Impl() {
    voidType = addType<VoidType>();
    boolType = addType<BoolType>();
    for (auto subtype: magic_enum::enum_values<IntSubType>()) {
        intTypes[subtype] = addType<IntType>(subtype);
    }
}

VoidType const* Context::voidType() { return impl->voidType; }

BoolType const* Context::boolType() { return impl->boolType; }

IntType const* Context::intType(IntSubType subtype) {
    return impl->intTypes[subtype];
}

VectorType const* Context::vectorType(int numComponents,
                                      VectorTypeFlags flags) {
    assert(numComponents >= 1);
    assert(numComponents <= 4);
    return impl->getOrInsert(impl->vectorTypes, { numComponents, flags }, [&] {
        return impl->addType<VectorType>(numComponents, flags);
    });
}

MatrixType const* Context::matrixType(int rows, int columns) {
    return impl->getOrInsert(impl->matrixTypes, { rows, columns }, [&] {
        return impl->addType<MatrixType>(rows, columns);
    });
}

AggregateType const* Context::aggregateType(
    std::string name, std::span<AggregateElement const> elements,
    bool isExtern) {
    return impl->getOrInsert(impl->aggTypes, name, [&] {
        return impl->addType<AggregateType>(name,
                                            elements | ranges::to<std::vector>,
                                            isExtern);
    });
}

TextureType const* Context::textureType(TextureSubType subtype) {
    return impl->getOrInsert(impl->textureTypes, subtype, [&] {
        return impl->addType<TextureType>(subtype);
    });
}

FunctionType const* Context::functionType(
    Type const* returnType, std::span<Type const* const> argTypes) {
    return impl->getOrInsert(impl->funcTypes,
                             { returnType, argTypes | ToSmallVector<> }, [&] {
        return impl->addType<FunctionType>(returnType,
                                           argTypes | ranges::to<std::vector>);
    });
}

PointerType const* Context::pointerType(Type const* baseType, Mutability mut) {
    return impl->getOrInsert(impl->pointerTypes, { baseType, mut }, [&] {
        return impl->addType<PointerType>(baseType, mut);
    });
}

ArrayType const* Context::arrayType(Type const* elementType,
                                    IntConstant const* count) {
    return impl->getOrInsert(impl->arrayTypes, { elementType, count }, [&] {
        return impl->addType<ArrayType>(elementType, count);
    });
}

ArrayType const* Context::arrayType(Type const* elementType, int count) {
    return arrayType(elementType, intConstant(IntSubType::I32, count));
}

std::string makeIntConstantName(IntSubType subtype, long long value) {
    return utl::strcat(magic_enum::enum_name(subtype), "(", value, ")");
}

IntConstant* Context::intConstant(IntSubType subtype, long long value) {

    return impl->getOrInsert(impl->intConstants, { subtype, value }, [&] {
        auto name = makeIntConstantName(subtype, value);
        return impl->addValue<IntConstant>(std::move(name), intType(subtype),
                                           value);
    });
}

static std::string makeVectorConstantName(int numComponents, float4 value) {
    std::stringstream sstr;
    sstr << "VecConst(";
    bool first = true;
    for (int i = 0; i < numComponents; ++i) {
        if (!first) {
            sstr << ", ";
        }
        first = false;
        sstr << value[i];
    }
    sstr << ")";
    return std::move(sstr).str();
}

VectorConstant* Context::vectorConstant(int numComponents, float4 value) {
    return impl->getOrInsert(impl->vectorConstants, { numComponents, value },
                             [&] {
        auto name = makeVectorConstantName(numComponents, value);
        return impl->addValue<VectorConstant>(*this, std::move(name),
                                              numComponents, value);
    });
}

VectorConstant* Context::floatConstant(float value) {
    return vectorConstant(1, float4(value, 0, 0, 0));
}

VectorConstant* Context::float2Constant(vml::float2 value) {
    return vectorConstant(2, float4(value, 0, 0));
}

VectorConstant* Context::float3Constant(vml::float3 value) {
    return vectorConstant(3, float4(value, 0));
}

VectorConstant* Context::float4Constant(vml::float4 value) {
    return vectorConstant(4, value);
}

TextureConstant* Context::textureConstant(TextureSubType subtype,
                                          AssetHandle asset) {
    return impl->getOrInsert(impl->textureConstants, asset.ID(), [&] {
        return impl->addValue<TextureConstant>("TexConst", textureType(subtype),
                                               asset);
    });
}

RasterizerInput* Context::rasterizerInput(RasterizerInputKind kind) {
    return impl->getOrInsert(impl->rasterizerInputs, kind, [&] {
        return impl->addValue<RasterizerInput>(*this, "RasterData", kind);
    });
}

BuiltinFunction* Context::builtinFunction(BuiltinFunctionKind kind,
                                          std::span<Type const* const> args) {
    return impl->getOrInsert(impl->builtins, { kind, args | ToSmallVector<> },
                             [&] {
        return impl->addValue<BuiltinFunction>(*this, kind, args);
    });
}

std::vector<Type const*> Context::types() const {
    return impl->typeOwners | transform(ToAddress<Type const>) |
           ranges::to<std::vector>;
}

std::vector<TextureConstant*> Context::textureConstants() const {
    return impl->textureConstants | values | ranges::to<std::vector>;
}

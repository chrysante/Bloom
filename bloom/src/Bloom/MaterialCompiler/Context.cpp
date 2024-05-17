#include "Bloom/MaterialCompiler/Context.h"

#include <range/v3/algorithm.hpp>
#include <utl/hash.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Types.h"
#include "Bloom/MaterialCompiler/Values.h"

using namespace bloom;
using namespace ranges::views;
using namespace matc;
using namespace vml::short_types;

using Impl = Context::Impl;

namespace {

struct AggregateHash {
    size_t operator()(auto&& rng) const {
        return utl::hash_combine_range(ranges::begin(rng), ranges::end(rng));
    }
};

struct AggregateEqual {
    bool operator()(auto&& a, auto&& b) const { return ranges::equal(a, b); }
};

} // namespace

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
    VectorType const* vectorType[4]{};
    utl::hashmap<TextureSubType, TextureType const*> textureTypes;
    utl::hashmap<utl::small_vector<Type const*>, AggregateType const*,
                 AggregateHash, AggregateEqual>
        aggTypes;
    TextureType const* textureType = nullptr;
    // Values
    std::vector<DynUniquePtr<Value>> valueOwners;
    utl::hashmap<std::pair<int, float4>, VectorConstant*> vectorConstants;
    utl::hashmap<RasterizerInputKind, RasterizerInput*> rasterizerInputs;
    utl::hashmap<utl::uuid, TextureConstant*> textureConstants;
};

Context::Context(): impl(std::make_unique<Impl>()) {}

Context::Context(Context&&) noexcept = default;

Context& Context::operator=(Context&&) noexcept = default;

Context::~Context() = default;

Impl::Impl() {
    vectorType[0] = addType<VectorType>(1);
    vectorType[1] = addType<VectorType>(2);
    vectorType[2] = addType<VectorType>(3);
    vectorType[3] = addType<VectorType>(4);
}

AggregateType const* Context::aggregateType(
    std::span<Type const* const> elements) {
    return impl->getOrInsert(impl->aggTypes, elements | ToSmallVector<>, [&] {
        return impl->addType<AggregateType>(elements | ranges::to<std::vector>);
    });
}

TextureType const* Context::textureType(TextureSubType subtype) {
    return impl->getOrInsert(impl->textureTypes, subtype, [&] {
        return impl->addType<TextureType>(subtype);
    });
}

VectorType const* Context::vectorType(int numComponents) const {
    assert(numComponents >= 1);
    assert(numComponents <= 4);
    return impl->vectorType[numComponents - 1];
}

VectorConstant* Context::vectorConstant(int numComponents, float4 value) {
    return impl->getOrInsert(impl->vectorConstants, { numComponents, value },
                             [&] {
        return impl->addValue<VectorConstant>(*this, numComponents, value);
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
        return impl->addValue<TextureConstant>(textureType(subtype), asset);
    });
}

RasterizerInput* Context::rasterizerInput(RasterizerInputKind kind) {
    return impl->getOrInsert(impl->rasterizerInputs, kind, [&] {
        return impl->addValue<RasterizerInput>(*this, kind);
    });
}

std::vector<Type const*> Context::types() const {
    return impl->typeOwners | ToAddress<Type const> | ranges::to<std::vector>;
}

std::vector<TextureConstant const*> Context::textureConstants() const {
    return impl->textureConstants | values |
           ranges::to<std::vector<TextureConstant const*>>;
}

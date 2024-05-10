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

    // Types
    std::vector<DynUniquePtr<Type>> typeOwners;
    VectorType const* vectorType[4]{};
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
    textureType = addType<TextureType>();
}

AggregateType const* Context::aggregateType(
    std::span<Type const* const> elements) {
    auto itr = impl->aggTypes.find(elements);
    if (itr != impl->aggTypes.end()) {
        return itr->second;
    }
    auto* type =
        impl->addType<AggregateType>(elements | ranges::to<std::vector>);
    impl->aggTypes.insert({ elements | ToSmallVector<>, type });
    return type;
}

TextureType const* Context::textureType() { return impl->textureType; }

VectorType const* Context::vectorType(int numComponents) const {
    assert(numComponents >= 1);
    assert(numComponents <= 4);
    return impl->vectorType[numComponents - 1];
}

VectorConstant* Context::vectorConstant(int numComponents, float4 value) {
    auto itr = impl->vectorConstants.find({ numComponents, value });
    if (itr != impl->vectorConstants.end()) {
        return itr->second;
    }
    auto* result = impl->addValue<VectorConstant>(*this, numComponents, value);
    impl->vectorConstants.insert({ { numComponents, value }, result });
    return result;
}

TextureConstant* Context::textureConstant(AssetHandle asset) {
    auto itr = impl->textureConstants.find(asset.ID());
    if (itr != impl->textureConstants.end()) {
        return itr->second;
    }
    auto* result = impl->addValue<TextureConstant>(*this, asset);
    impl->textureConstants.insert({ asset.ID(), result });
    return result;
}

RasterizerInput* Context::rasterizerInput(RasterizerInputKind kind) {
    auto itr = impl->rasterizerInputs.find(kind);
    if (itr != impl->rasterizerInputs.end()) {
        return itr->second;
    }
    auto* result = impl->addValue<RasterizerInput>(*this, kind);
    impl->rasterizerInputs.insert({ kind, result });
    return result;
}

std::vector<Type const*> Context::types() const {
    return impl->typeOwners | ToAddress<Type const> | ranges::to<std::vector>;
}

std::vector<TextureConstant const*> Context::textureConstants() const {
    return impl->textureConstants | values |
           ranges::to<std::vector<TextureConstant const*>>;
}

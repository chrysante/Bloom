#include "Bloom/MaterialCompiler/Context.h"

#include <range/v3/algorithm.hpp>
#include <utl/hash.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/MaterialCompiler/Types.h"

using namespace bloom;
using namespace matgraph;

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

    std::vector<DynUniquePtr<Type>> typeOwners;
    FloatType const* floatType;
    Float2Type const* float2Type;
    Float3Type const* float3Type;
    Float4Type const* float4Type;
    utl::hashmap<utl::small_vector<Type const*>, AggregateType const*,
                 AggregateHash, AggregateEqual>
        aggTypes;
};

Context::Context(): impl(std::make_unique<Impl>()) {}

Context::Context(Context&&) noexcept = default;

Context& Context::operator=(Context&&) noexcept = default;

Context::~Context() = default;

Impl::Impl() {
    floatType = addType<FloatType>();
    float2Type = addType<Float2Type>();
    float3Type = addType<Float3Type>();
    float4Type = addType<Float4Type>();
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

FloatType const* Context::floatType() const { return impl->floatType; }

Float2Type const* Context::float2Type() const { return impl->float2Type; }

Float3Type const* Context::float3Type() const { return impl->float3Type; }

Float4Type const* Context::float4Type() const { return impl->float4Type; }

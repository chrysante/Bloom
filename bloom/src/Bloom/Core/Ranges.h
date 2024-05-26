#ifndef BLOOM_CORE_RANGES_H
#define BLOOM_CORE_RANGES_H

#include <memory>
#include <type_traits>

#include <range/v3/view.hpp>
#include <utl/vector.hpp>

namespace bloom {

template <size_t N>
struct ToSmallVectorImpl {};

template <ranges::range R, size_t N>
inline constexpr auto operator|(R&& rng, ToSmallVectorImpl<N>) {
    using T = ranges::range_value_t<R>;
    if constexpr (N == ~size_t{}) {
        return std::forward<R>(rng) | ranges::to<utl::small_vector<T>>;
    }
    else {
        return std::forward<R>(rng) | ranges::to<utl::small_vector<T, N>>;
    }
}

template <size_t N = ~size_t{}>
inline constexpr ToSmallVectorImpl<N> ToSmallVector{};

} // namespace bloom

#endif // BLOOM_CORE_RANGES_H

#ifndef BLOOM_CORE_RANGES_H
#define BLOOM_CORE_RANGES_H

#include <memory>
#include <type_traits>

#include <range/v3/view.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Dyncast.h"

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

template <typename... T>
    requires(sizeof...(T) > 0)
inline constexpr auto Filter =
    ranges::views::filter([]<typename V>(V const& value) {
    using NoRef = std::remove_reference_t<V>;
    if constexpr (std::is_pointer_v<NoRef>) {
        return (... || isa<T>(value));
    }
    else {
        return (... || isa<T>(value));
    }
}) | ranges::views::transform([]<typename V>(V&& value) -> decltype(auto) {
    if constexpr (sizeof...(T) > 1) {
        return std::forward<V>(value);
    }
    else {
        using NoRef = std::remove_reference_t<V>;
        using NoPtr = std::remove_pointer_t<NoRef>;
        using First = std::tuple_element_t<0, std::tuple<T...>>;
        if constexpr (std::is_pointer_v<NoRef>) {
            return cast<utl::copy_cv_t<NoPtr, First>*>(value);
        }
        else {
            return cast<utl::copy_cv_t<NoPtr, First>&>(value);
        }
    }
});

} // namespace bloom

#endif // BLOOM_CORE_RANGES_H

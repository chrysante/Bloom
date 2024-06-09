#ifndef BLOOM_CORE_ENUMCOUNT_H
#define BLOOM_CORE_ENUMCOUNT_H

#include <array>
#include <type_traits>

#include <magic_enum.hpp>

#include "Bloom/Core/Debug.h"

namespace bloom {

template <typename E>
    requires std::is_enum_v<E>
inline constexpr std::size_t EnumCount = magic_enum::enum_count<E>();

/// Wrapper around `std::array` that has as many elements are there are cases in
/// `E` and that can be indexed with values from `E`. This relies on the
/// `EnumCount` template and `E` being well behaved
template <typename E, typename T>
struct EnumArray: std::array<T, EnumCount<E>> {
    static_assert(sizeof(EnumCount<E>));

    using std::array<T, EnumCount<E>>::operator[];

    T& operator[](E e) { return (*this)[(size_t)e]; }

    T const& operator[](E e) const { return (*this)[(size_t)e]; }
};

template <typename E, typename T>
constexpr EnumArray<E, T> makeEnumArray(T const& value) {
    return [&]<size_t... I>(std::index_sequence<I...>) {
        return EnumArray<E, T>{ ((void)I, value)... };
    }(std::make_index_sequence<EnumCount<E>>{});
}

} // namespace bloom

#endif // BLOOM_CORE_ENUMCOUNT_H

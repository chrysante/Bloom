#ifndef BLOOM_CORE_ENUMCOUNT_H
#define BLOOM_CORE_ENUMCOUNT_H

#include <array>
#include <type_traits>

#include "Bloom/Core/Debug.h"

namespace bloom {

template <typename E>
    requires std::is_enum_v<E>
inline constexpr std::size_t EnumCount = []() -> std::size_t {
    if constexpr (requires { E::COUNT; }) {
        return (std::size_t)E::COUNT;
    }
    else if constexpr (requires { E::LAST; }) {
        return 1 + (std::size_t)E::LAST;
    }
    else {
        static_assert(!std::is_same_v<E, E>,
                      "EnumCount requires either E::COUNT or E::LAST");
        BL_UNREACHABLE();
    }
}();

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

} // namespace bloom

#endif // BLOOM_CORE_ENUMCOUNT_H

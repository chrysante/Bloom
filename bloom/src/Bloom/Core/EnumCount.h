#ifndef BLOOM_CORE_ENUMCOUNT_H
#define BLOOM_CORE_ENUMCOUNT_H

#include <type_traits>

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
    }
}();

} // namespace bloom

#endif // BLOOM_CORE_ENUMCOUNT_H

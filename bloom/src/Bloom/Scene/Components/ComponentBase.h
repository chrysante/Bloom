#ifndef BLOOM_SCENE_COMPONENTS_COMPONENTBASE_H
#define BLOOM_SCENE_COMPONENTS_COMPONENTBASE_H

#include "Bloom/Core/Base.h"

#include <type_traits>

#define BLOOM_REGISTER_COMPONENT(NAME)                                         \
    static constexpr char const* staticName() { return NAME; }                 \
    using __bloom_is_component = std::true_type;

namespace bloom {

template <typename T>
concept ComponentType =
    requires { typename std::remove_reference_t<T>::__bloom_is_component; };

template <typename T>
struct IsComponent: std::bool_constant<ComponentType<T>> {};

} // namespace bloom

#endif // BLOOM_SCENE_COMPONENTS_COMPONENTBASE_H

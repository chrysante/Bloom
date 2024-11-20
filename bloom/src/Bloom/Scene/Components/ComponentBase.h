#ifndef BLOOM_SCENE_COMPONENTS_COMPONENTBASE_H
#define BLOOM_SCENE_COMPONENTS_COMPONENTBASE_H

#include <tuple>
#include <type_traits>

#include <utl/common.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Utils.h"

namespace bloom {

/// Forward declaration of all component types
#define BLOOM_COMPONENT_DEF(Type, ...) struct Type;
#include "Bloom/Scene/Components/Components.def"

/// List of all component types
enum class ComponentKind {
#define BLOOM_COMPONENT_DEF(Type, ...) Type,
#include "Bloom/Scene/Components/Components.def"
};

/// We need to ignore the last comma to form the tuple of all components, that's
/// why we need this stupid dance
template <typename Ignore, typename... C>
struct AllComponentsImpl: std::type_identity<TypeList<C...>> {};

/// Tuple of all component types
using AllComponents = AllComponentsImpl<void
#define BLOOM_COMPONENT_DEF(Type, ...) , Type
#include "Bloom/Scene/Components/Components.def"
                                        >::type;

/// Maps static component types to dynamic types
template <typename>
inline constexpr ComponentKind DynamicType = ComponentKind(-1);

#define BLOOM_COMPONENT_DEF(Type, ...)                                         \
    template <>                                                                \
    inline constexpr ComponentKind DynamicType<Type> = ComponentKind::Type;
#include "Bloom/Scene/Components/Components.def"

///
char const* componentName(ComponentKind kind);

/// Type traits that has a static member variable `value` set to `true` if \p T
/// is a component type
template <typename T>
struct IsComponent: std::false_type {};

#define BLOOM_COMPONENT_DEF(Type, ...)                                         \
    template <>                                                                \
    struct IsComponent<Type>: std::true_type {};
#include "Bloom/Scene/Components/Components.def"

/// Concept evaluating to `true` if \p T is a component type
template <typename T>
concept ComponentType = IsComponent<std::remove_cvref_t<T>>::value;

/// Concept evaluating to `true` if \p T is a non-empty component type
template <typename T>
concept DataComponent = ComponentType<T> && !std::is_empty_v<T>;

///
template <typename T>
struct IsDataComponent: std::bool_constant<DataComponent<T>> {};

/// Metafunction that transforms components into their weak counterparts
template <typename T>
struct ToWeakComponent: std::type_identity<T> {};
template <typename T>
    requires requires { typename T::WeakType; }
struct ToWeakComponent<T>: std::type_identity<typename T::WeakType> {};

/// Metafunction that transforms weak components into their strong (default)
/// counterparts
template <typename T>
struct ToStrongComponent: std::type_identity<T> {};
template <typename T>
    requires requires { typename T::StrongType; }
struct ToStrongComponent<T>: std::type_identity<typename T::StrongType> {};

template <typename>
struct IsComponentList: std::false_type {};

template <ComponentType... T>
struct IsComponentList<TypeList<T...>>: std::true_type {};

template <typename L>
concept ComponentList = IsComponentList<L>::value;

template <typename... T>
struct Except: TypeList<T...> {};

template <typename... T>
constexpr auto except = Except<T...>{};

/// Invokes \p fn with an argument of type `utl::tag<C>` for all component types
/// `C` except for for the types listed in `Exclude...`
template <typename... Exclude>
void forEachComponent(Except<Exclude...>, auto&& fn) {
    auto invokeOne = [&]<typename T>(utl::tag<T>) {
        if constexpr (((!std::is_same_v<T, Exclude>) && ...)) {
            std::invoke(fn, utl::tag<T>{});
        }
    };
    [&]<typename... T>(TypeList<T...>) {
        (invokeOne(utl::tag<T>{}), ...);
    }(AllComponents{});
}

/// \overload
void forEachComponent(auto&& fn) {
    forEachComponent(Except<>{}, UTL_FORWARD(fn));
}

/// Invokes \p fn with an argument of type `utl::tag<C>` where `C` is the static
/// type corresponding to \p dynamicKind
decltype(auto) visitComponentType(ComponentKind dynamicKind, auto&& fn) {
    switch (dynamicKind) {
#define BLOOM_COMPONENT_DEF(Type, ...)                                         \
    case ComponentKind::Type:                                                  \
        return std::invoke(fn, utl::tag<bloom::Type>{});
#include "Bloom/Scene/Components/Components.def"
    }
}

/// Visit a subset of components
template <typename First, typename... Rest>
decltype(auto) visitComponentTypeImpl(ComponentKind dynamicKind, auto&& fn) {
    if (DynamicType<First> == dynamicKind) {
        return std::invoke(fn, utl::tag<First>{});
    }
    else if constexpr (sizeof...(Rest) != 0) {
        return visitComponentTypeImpl<Rest...>(dynamicKind, fn);
    }
    else {
        BL_UNREACHABLE();
    }
}

/// \overload
template <ComponentList Types>
decltype(auto) visitComponentType(ComponentKind dynamicKind, auto&& fn) {
    return [&]<typename... T>(TypeList<T...>) {
        return visitComponentTypeImpl<T...>(dynamicKind, fn);
    }(Types{});
}

} // namespace bloom

#endif // BLOOM_SCENE_COMPONENTS_COMPONENTBASE_H

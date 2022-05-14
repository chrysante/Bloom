#pragma once

#include "Bloom/Core/Base.hpp"

#include <type_traits>

#define BLOOM_REGISTER_COMPONENT(NAME) \
static constexpr char const* staticName() { return NAME; } \
struct __bloom_is_component{};

namespace bloom {
	
	template <typename T>
	concept Component = requires { typename std::remove_reference_t<T>::__bloom_is_component; };
	
	template <typename T>
	struct IsComponent: std::bool_constant<Component<T>>{};
	
}

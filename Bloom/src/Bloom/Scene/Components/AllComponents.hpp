#pragma once

#include "Tag.hpp"
#include "Transform.hpp"
#include "MeshRenderer.hpp"
#include "Hierarchy.hpp"
#include "Lights.hpp"

#include <tuple>

namespace bloom {

	using ComponentList = std::tuple<
		TagComponent,
		TransformComponent,
		TransformMatrixComponent,
		HierarchyComponent,
		MeshRendererComponent,
		PointLightComponent,
		SpotLightComponent,
		DirectionalLightComponent,
		SkyLightComponent
	>;

	template <typename...T>
	struct Except: std::tuple<T...> {};
	template <typename... T>
	constexpr auto except = Except<T...>{};

	template <typename F, typename... E>
	void forEachComponent(Except<E...>, F&& f) {
		auto invokeOne = [&]<typename T>(utl::tag<T>) {
			if constexpr (((!std::is_same_v<T, E>) && ...)) {
				std::invoke(f, utl::tag<T>{});
			}
		};
		UTL_WITH_INDEX_SEQUENCE((I, std::tuple_size_v<ComponentList>), {
			(invokeOne(utl::tag<std::tuple_element_t<I, ComponentList>>{}), ...);
		});
	}

	template <typename F>
	void forEachComponent(F&& f) {
		forEachComponent(except<>, UTL_FORWARD(f));
	}

}


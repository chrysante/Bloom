#ifndef BLOOM_SCENE_COMPONENTS_ALLCOMPONENTS_H
#define BLOOM_SCENE_COMPONENTS_ALLCOMPONENTS_H

#include <tuple>

#include "Bloom/Scene/Components/Hierarchy.h"
#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/MeshRenderer.h"
#include "Bloom/Scene/Components/Script.h"
#include "Bloom/Scene/Components/Tag.h"
#include "Bloom/Scene/Components/Transform.h"

namespace bloom {

using ComponentList =
    std::tuple<TagComponent, Transform, TransformMatrixComponent,
               HierarchyComponent, ScriptComponent, ScriptPreservedData,
               MeshRendererComponent, PointLightComponent, SpotLightComponent,
               DirectionalLightComponent, SkyLightComponent>;

template <typename... T>
struct Except: std::tuple<T...> {};

template <typename... T>
constexpr auto except = Except<T...>{};

template <typename F, typename... E>
void forEachComponent(Except<E...>, F&& f) {
    auto invokeOne = [&]<typename T>(utl::tag<T>) {
        if constexpr (((!std::is_same_v<T, E>)&&...)) {
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

} // namespace bloom

#endif // BLOOM_SCENE_COMPONENTS_ALLCOMPONENTS_H

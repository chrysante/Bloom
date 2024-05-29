#include "Poppy/Views/BasicSceneInspector.h"

#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/View.h"

using namespace bloom;
using namespace poppy;

std::string poppy::toString(LightType type) {
    return std::array{ "Point Light", "Spotlight", "Directional Light",
                       "Sky Light" }[(size_t)type];
}

std::string poppy::toString(ColliderType type) {
    return std::array{ "Sphere Collider", "Box Collider" }[(size_t)type];
}

template <ComponentType... T>
static std::optional<ComponentKind> getAnyOfSubset(EntityHandle entity) {
    std::optional<ComponentKind> result;
    (void)((entity.has<T>() ? ((result = DynamicType<T>), true) : false) ||
           ...);
    return result;
}

std::optional<ComponentKind> poppy::getLightType2(EntityHandle entity) {
    return getAnyOfSubset<PointLightComponent, SpotLightComponent,
                          DirectionalLightComponent, SkyLightComponent>(entity);
}

std::optional<ComponentKind> poppy::getPhysicsType2(EntityHandle entity) {
    return getAnyOfSubset<PhysicsStaticComponent, PhysicsMovableComponent>(
        entity);
}

std::optional<ComponentKind> poppy::getColliderType2(EntityHandle entity) {
    return getAnyOfSubset<SphereColliderComponent, BoxColliderComponent>(
        entity);
}

std::span<Scene* const> BasicSceneInspector::scenes() const {
    return view->editor().coreSystems().sceneSystem().scenes();
}

EntitySelectionManager& BasicSceneInspector::selection() const {
    return view->editor().selection();
}

AssetManager& BasicSceneInspector::assetManager() const {
    return view->editor().coreSystems().assetManager();
}

bool BasicSceneInspector::hasLightComponent(EntityHandle entity) const {
    return getLightType(entity).has_value();
}

bool BasicSceneInspector::isSimulating() const {
    return view->editor().isSimulating();
}

std::optional<LightType> BasicSceneInspector::getLightType(
    EntityHandle entity) const {
    if (entity.has<PointLightComponent>()) {
        return LightType::PointLight;
    }
    if (entity.has<SpotLightComponent>()) {
        return LightType::SpotLight;
    }
    if (entity.has<DirectionalLightComponent>()) {
        return LightType::DirectionalLight;
    }
    if (entity.has<SkyLightComponent>()) {
        return LightType::SkyLight;
    }
    return std::nullopt;
}

void BasicSceneInspector::removeComponentType(LightType type,
                                              EntityHandle entity) {
    dispatchComponentType(type,
                          [&]<typename T>(utl::tag<T>) { entity.remove<T>(); });
}

std::optional<PhysicsComponentType> BasicSceneInspector::getPhysicsType(
    EntityHandle entity) const {
    if (entity.has<PhysicsStaticComponent>()) {
        return PhysicsComponentType::Static;
    }
    if (entity.has<PhysicsMovableComponent>()) {
        return PhysicsComponentType::Movable;
    }
    return std::nullopt;
}

void BasicSceneInspector::removeComponentType(PhysicsComponentType type,
                                              EntityHandle entity) {
    dispatchComponentType(type,
                          [&]<typename T>(utl::tag<T>) { entity.remove<T>(); });
}

bool BasicSceneInspector::hasColliderComponent(EntityHandle entity) const {
    return getColliderType(entity).has_value();
}

std::optional<ColliderType> BasicSceneInspector::getColliderType(
    EntityHandle entity) const {
    if (entity.has<SphereColliderComponent>()) {
        return ColliderType::Sphere;
    }
    if (entity.has<BoxColliderComponent>()) {
        return ColliderType::Box;
    }
    return std::nullopt;
}

void BasicSceneInspector::removeComponentType(ColliderType type,
                                              EntityHandle entity) {
    dispatchComponentType(type,
                          [&]<typename T>(utl::tag<T>) { entity.remove<T>(); });
}

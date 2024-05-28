#ifndef POPPY_VIEWS_BASICSCENEINSPECTOR_H
#define POPPY_VIEWS_BASICSCENEINSPECTOR_H

#include <optional>
#include <span>
#include <string>

#include <utl/vector.hpp>

#include "Bloom/Graphics/Lights.h"
#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/PhysicsComponents.h"
#include "Bloom/Scene/Entity.h"
#include "Poppy/Core/Debug.h"

namespace bloom {

class Scene;
class AssetManager;
class Application;

} // namespace bloom

namespace poppy {

class Editor;
class View;
class EntitySelectionManager;

enum struct LightType {
    PointLight = 0,
    SpotLight = 1,
    DirectionalLight = 2,
    SkyLight = 3,
    LAST = SkyLight
};

std::string toString(LightType type);

enum struct ColliderType { Sphere = 0, Box = 1 };

std::string toString(ColliderType type);

decltype(auto) dispatchComponentType(LightType type, auto&& f) {
    using namespace bloom;
    switch (type) {
    case LightType::PointLight:
        return f(utl::tag<PointLightComponent>{});
    case LightType::SpotLight:
        return f(utl::tag<SpotLightComponent>{});
    case LightType::DirectionalLight:
        return f(utl::tag<DirectionalLightComponent>{});
    case LightType::SkyLight:
        return f(utl::tag<SkyLightComponent>{});
    }
}

decltype(auto) dispatchComponentType(ColliderType type, auto&& f) {
    using namespace bloom;
    switch (type) {
    case ColliderType::Sphere:
        return f(utl::tag<SphereColliderComponent>{});
    case ColliderType::Box:
        return f(utl::tag<BoxColliderComponent>{});
    }
}

decltype(auto) dispatchComponentType(bloom::PhysicsComponentType type,
                                     auto&& f) {
    using namespace bloom;
    switch (type) {
    case PhysicsComponentType::Static:
        return f(utl::tag<PhysicsStaticComponent>{});
    case PhysicsComponentType::Movable:
        return f(utl::tag<PhysicsMovableComponent>{});
    }
}

class BasicSceneInspector {
protected:
    BasicSceneInspector(View* view): view(view) {}

public:
    std::span<bloom::Scene* const> scenes() const;

    EntitySelectionManager& selection() const;

    bloom::AssetManager& assetManager() const;

    bool isSimulating() const;

    bool hasLightComponent(bloom::EntityHandle entity) const;

    std::optional<LightType> getLightType(bloom::EntityHandle entity) const;

    void removeComponentType(LightType type, bloom::EntityHandle entity);

    std::optional<bloom::PhysicsComponentType> getPhysicsType(
        bloom::EntityHandle entity) const;

    void removeComponentType(bloom::PhysicsComponentType type,
                             bloom::EntityHandle entity);

    bool hasColliderComponent(bloom::EntityHandle entity) const;

    std::optional<ColliderType> getColliderType(
        bloom::EntityHandle entity) const;

    void removeComponentType(ColliderType type, bloom::EntityHandle entity);

private:
    View* view = nullptr;
};

} // namespace poppy

#endif // POPPY_VIEWS_BASICSCENEINSPECTOR_H

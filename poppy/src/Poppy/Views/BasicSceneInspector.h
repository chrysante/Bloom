#ifndef POPPY_VIEWS_BASICSCENEINSPECTOR_H
#define POPPY_VIEWS_BASICSCENEINSPECTOR_H

#include <optional>
#include <span>
#include <string>

#include <utl/vector.hpp>

#include "Bloom/Graphics/Lights.h"
#include "Bloom/Scene/Components/Lights.h"
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

decltype(auto) dispatchLightComponent(LightType type, auto&& f) {
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
    default:
        BL_UNREACHABLE();
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

    bool hasLightComponent(bloom::EntityHandle) const;

    std::optional<LightType> getLightType(bloom::EntityHandle) const;

    void removeLightComponent(LightType, bloom::EntityHandle);

private:
    View* view = nullptr;
};

} // namespace poppy

#endif // POPPY_VIEWS_BASICSCENEINSPECTOR_H

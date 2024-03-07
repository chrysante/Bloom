#pragma once

#include <span>

#include <utl/vector.hpp>

#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Scene/Components/Lights.hpp"
#include "Bloom/Scene/Entity.hpp"
#include "Poppy/Core/Debug.hpp"
#include "Poppy/UI/View.hpp"

namespace bloom {

class Scene;
class AssetManager;
class Application;

} // namespace bloom

namespace poppy {

class Editor;
class SelectionContext;

enum struct LightType {
    none = -1,
    pointlight = 0,
    spotlight = 1,
    directional = 2,
    skylight = 3,
    _count
};

std::string_view toString(LightType);

auto dispatchLightComponent(LightType type, auto&& f) {
    using namespace bloom;
    switch (type) {
    case LightType::pointlight:
        return f(utl::tag<PointLightComponent>{});
    case LightType::spotlight:
        return f(utl::tag<SpotLightComponent>{});
    case LightType::directional:
        return f(utl::tag<DirectionalLightComponent>{});
    case LightType::skylight:
        return f(utl::tag<SkyLightComponent>{});
    default:
        BL_DEBUGFAIL();
    }
}

class BasicSceneInspector {
protected:
    BasicSceneInspector(View* view): view(view) {}

public:
    std::span<bloom::Scene* const> scenes() const;

    SelectionContext& selection() const;

    bloom::AssetManager& assetManager() const;

    bool isSimulating() const;

    bool hasLightComponent(bloom::EntityHandle) const;

    LightType getLightType(bloom::EntityHandle) const;

    bloom::LightCommon getLightCommon(LightType, bloom::EntityHandle) const;

    void removeLightComponent(LightType, bloom::EntityHandle);

private:
    View* view = nullptr;
};

} // namespace poppy

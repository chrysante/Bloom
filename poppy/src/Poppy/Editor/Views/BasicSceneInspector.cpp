#include "Poppy/Editor/Views/BasicSceneInspector.h"

#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/View.h"

using namespace bloom;
using namespace poppy;

std::string_view poppy::toString(LightType type) {
    return std::array{ "Point Light", "Spotlight", "Directional Light",
                       "Sky Light" }[(std::size_t)type];
}

std::span<Scene* const> BasicSceneInspector::scenes() const {
    return view->editor().coreSystems().sceneSystem().scenes();
}

SelectionContext& BasicSceneInspector::selection() const {
    return view->editor().selection();
}

bloom::AssetManager& BasicSceneInspector::assetManager() const {
    return view->editor().coreSystems().assetManager();
}

bool BasicSceneInspector::hasLightComponent(bloom::EntityHandle entity) const {
    return getLightType(entity) != LightType::none;
}

bool BasicSceneInspector::isSimulating() const {
    return view->editor().isSimulating();
}

LightType BasicSceneInspector::getLightType(bloom::EntityHandle entity) const {
    if (entity.has<PointLightComponent>()) {
        return LightType::pointlight;
    }
    if (entity.has<SpotLightComponent>()) {
        return LightType::spotlight;
    }
    if (entity.has<DirectionalLightComponent>()) {
        return LightType::directional;
    }
    if (entity.has<SkyLightComponent>()) {
        return LightType::skylight;
    }
    return LightType::none;
}

bloom::LightCommon BasicSceneInspector::getLightCommon(
    LightType type, bloom::EntityHandle entity) const {
    return dispatchLightComponent(type, [&]<typename T>(utl::tag<T>) {
        auto const result = entity.get<T>().light.common;
        return result;
    });
}

void BasicSceneInspector::removeLightComponent(LightType type,
                                               bloom::EntityHandle entity) {
    dispatchLightComponent(type, [&]<typename T>(utl::tag<T>) {
        entity.remove<T>();
    });
}

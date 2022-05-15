#include "BasicSceneInspector.hpp"

#include "Poppy/Debug.hpp"

#include "Bloom/Assets/ConcreteAssets.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components/AllComponents.hpp"
#include "Poppy/Editor.hpp"

using namespace bloom;

namespace poppy {
	
	std::string_view toString(LightType type) {
		return std::array {
			"Point Light",
			"Spotlight",
			"Directional Light",
			"Sky Light"
		}[(std::size_t)type];
	}
	
	bloom::Scene*        BasicSceneInspector::scene()        const {
		return editor->scene ? &editor->scene->scene : nullptr;
	}
	
	SelectionContext*    BasicSceneInspector::selection()    const {
		return &editor->selection;
	}
	
	bloom::AssetManager* BasicSceneInspector::assetManager() const {
		return &editor->assetManager();
	}
	
	bool BasicSceneInspector::hasLightComponent(bloom::EntityID entity) const {
		return getLightType(entity) != LightType::none;
	}
	
	LightType BasicSceneInspector::getLightType(bloom::EntityID id) const {
		auto const entity = scene()->getHandle(id);
		
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
	
	bloom::LightCommon BasicSceneInspector::getLightCommon(LightType type, bloom::EntityID entity) const {
		auto impl = [&]<typename T>() {
			auto const result = scene()->getComponent<T>(entity).light.common;
			return result;
		};
		switch (type) {
			case LightType::pointlight:
				return impl.operator()<PointLightComponent>();
			case LightType::spotlight:
				return impl.operator()<SpotLightComponent>();
			case LightType::directional:
				return impl.operator()<DirectionalLightComponent>();
			default:
				poppyDebugbreak();
				std::terminate();
		}
	}
	
	void BasicSceneInspector::removeLightComponent(LightType type, bloom::EntityID entity) {
		auto impl = [&]<typename T>() {
			scene()->removeComponent<T>(entity);
		};
		switch (type) {
			case LightType::pointlight:
				return impl.operator()<PointLightComponent>();
			case LightType::spotlight:
				return impl.operator()<SpotLightComponent>();
			case LightType::directional:
				return impl.operator()<DirectionalLightComponent>();
			default:
				poppyDebugbreak();
				std::terminate();
		}
	}
	
}

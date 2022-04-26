#include "Components.hpp"

namespace bloom {
	
	/// MARK: TransformComponent
	mtl::float4x4 TransformComponent::calculate() const {
		return mtl::make_transform(position, orientation, scale);
	}
	
	
	/// MARK: LightComponent
	LightCommon const& LightComponent::getCommon() const {
		switch (type()) {
			case LightType::pointlight: return get<PointLight>().common;
			case LightType::spotlight: return get<SpotLight>().common;
			case LightType::directional: return get<DirectionalLight>().common;
			default: bloomDebugbreak(); std::terminate();
		}
	}
	
	bool LightComponent::castsShadows() const {
		switch (type()) {
			case LightType::pointlight: return false;
			case LightType::spotlight: return false;
			case LightType::directional: return get<DirectionalLight>().castsShadows;
			default: bloomDebugbreak(); std::terminate();
		}
	}
	
}

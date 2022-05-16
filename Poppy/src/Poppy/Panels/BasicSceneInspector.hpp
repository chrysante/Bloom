#pragma once

#include <utl/vector.hpp>
#include "Bloom/Scene/Entity.hpp"
#include "Bloom/Graphics/Lights.hpp"

namespace bloom {
	class Scene;
	class AssetManager;
	class Application;
}
namespace poppy {
	class Editor;
	class SelectionContext;
}

namespace poppy {
	
	enum struct LightType {
		none = -1,
		pointlight = 0,
		spotlight = 1,
		directional = 2,
		skylight = 3,
		_count
	};

	std::string_view toString(LightType);
	
	class BasicSceneInspector {
		friend class Editor;
		
	public:
		bloom::Scene*        scene()        const;
		SelectionContext*    selection()    const;
		bloom::AssetManager* assetManager() const;
		
		bool hasLightComponent(bloom::EntityID) const;
		LightType getLightType(bloom::EntityID) const;
		bloom::LightCommon getLightCommon(LightType, bloom::EntityID) const;
		void removeLightComponent(LightType, bloom::EntityID);
	};
	
}

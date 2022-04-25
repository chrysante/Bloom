#pragma once

#include <utl/vector.hpp>
#include "Bloom/Scene/Entity.hpp"

namespace bloom {
	class Scene;
	class AssetManager;
}
namespace poppy {
	class SelectionContext;
}

namespace poppy {
	
	class BasicSceneInspector {
	public:
		void setScene(bloom::Scene*);
		void setSelectionContext(SelectionContext*);
		void setAssetManager(bloom::AssetManager*);
		
		bloom::Scene*        scene()        const { return _scene;        }
		SelectionContext*    selection()    const { return _selection;    }
		bloom::AssetManager* assetManager() const { return _assetManager; }
		
	private:
		bloom::Scene* _scene = nullptr;
		SelectionContext* _selection = nullptr;
		bloom::AssetManager* _assetManager = nullptr;
	};
	
}

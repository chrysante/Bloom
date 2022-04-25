#include "BasicSceneInspector.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components.hpp"

namespace poppy {
	
	void BasicSceneInspector::setScene(bloom::Scene* s) {
		_scene = s;
	}
	
	void BasicSceneInspector::setSelectionContext(SelectionContext* s) {
		_selection = s;
	}
	
	void BasicSceneInspector::setAssetManager(bloom::AssetManager* m) {
		_assetManager = m;
	}
	
}

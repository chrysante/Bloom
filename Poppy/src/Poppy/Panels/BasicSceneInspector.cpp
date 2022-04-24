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
	
	utl::small_vector<bloom::EntityID> BasicSceneInspector::gatherRootEntities() {
		utl::small_vector<bloom::EntityID> roots;
		for (auto [entity, hierachy]: scene()->view<bloom::HierachyComponent>().each()) {
			if (!hierachy.parent) {
				roots.push_back(bloom::EntityID(entity));
			}
		}
		return roots;
	}
	
	utl::small_vector<bloom::EntityID> BasicSceneInspector::gatherChildren(bloom::EntityID parent) {
		if (!scene()->hasComponent<bloom::HierachyComponent>(parent)) {
			return {};
		}
		auto const& hierachy = scene()->getComponent<bloom::HierachyComponent>(parent);
		auto current = hierachy.childLeft;
		auto const end = hierachy.childRight;
		
		utl::small_vector<bloom::EntityID> children;
		while (current) {
			children.push_back(current);
			if (current == end) {
				break;
			}
			current = scene()->getComponent<bloom::HierachyComponent>(current).siblingRight;
		}
		
		return children;
	}
	
	
	
}

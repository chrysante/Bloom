#pragma once

#include "Panel.hpp"
#include "BasicSceneInspector.hpp"

#include "Bloom/Scene/Entity.hpp"

#include <utl/vector.hpp>
#include <span>
#include <optional>

namespace poppy {
	
	class SceneInspector: public Panel, public BasicSceneInspector {
	public:
		SceneInspector();
		
	private:
		void display() override;
		
		
		void displayEntity(bloom::EntityID);

		void dragDropSource(bloom::EntityID child);
		void dragDropTarget(bloom::EntityID parent);
		
		bool expanded(bloom::EntityID) const;
		void setExpanded(bloom::EntityID, bool);
		
	private:
		mutable utl::vector<bool> _expanded;
		
		std::pair<bloom::EntityID, bloom::EntityID> hierarchyUpdate;
		bool hasHierarchyUpdate = 0;
	};
	
	
}

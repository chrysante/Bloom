#pragma once

#include "Poppy/UI/View.hpp"
#include "PropertiesView.hpp"
#include "BasicSceneInspector.hpp"

#include "Bloom/Scene/Entity.hpp"

#include <utl/vector.hpp>
#include <utl/hashset.hpp>
#include <span>
#include <optional>

namespace bloom { class Scene; }

namespace poppy {
	
	class SceneOutliner:
		public View,
		public BasicSceneInspector
	{
	public:
		SceneOutliner();
		
	private:
		void frame() override;
		
		struct TreeNodeDescription {
			std::size_t id = 0;
			bool selected = false;
			bool expanded = false;
			bool isLeaf = true;
			std::string_view name;
		};
		void treeNode(TreeNodeDescription const&, auto&& block);
		void displayEntity(bloom::EntityHandle);
		void displayScene(bloom::Scene&);

		void dragDropSource(bloom::EntityHandle child);
		void dragDropTarget(bloom::EntityHandle parent);
		
		bool expanded(bloom::Scene const*) const;
		bool expanded(bloom::ConstEntityHandle) const;
		void setExpanded(bloom::Scene const*, bool);
		void setExpanded(bloom::ConstEntityHandle, bool);
		
		
		void performUpdates();
		void performHierarchyUpdate();
		void deferHierarchyUpdate(bloom::EntityHandle child, bloom::EntityHandle parent);
		
		void performDeletion();
		
	private:
		utl::hashset<bloom::ConstEntityHandle> mExpanded;
		
		std::pair<bloom::EntityHandle, bloom::EntityHandle> hierarchyUpdate;
		bool hasHierarchyUpdate = 0;
		
		bloom::EntityHandle toDelete;
	};
	
	
}

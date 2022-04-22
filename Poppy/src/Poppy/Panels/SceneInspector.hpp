#pragma once

#include "Panel.hpp"

#include "Bloom/Scene/Scene.hpp"

#include <utl/vector.hpp>
#include <span>

namespace poppy {
	
	class SelectionContext;
	
	class SceneInspector: public Panel {
	public:
		SceneInspector(bloom::Scene*, SelectionContext*);
		
	private:
		void display() override;
		void displayHierachyLevel(std::span<bloom::EntityID const>);
		
		utl::small_vector<bloom::EntityID> gatherRootEntities();
		
	private:
		bloom::Scene* scene;
		SelectionContext* selection;
	};
	
	
}

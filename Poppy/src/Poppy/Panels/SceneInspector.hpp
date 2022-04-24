#pragma once

#include "Panel.hpp"
#include "BasicSceneInspector.hpp"

#include "Bloom/Scene/Entity.hpp"

#include <utl/vector.hpp>
#include <span>

namespace poppy {
	
	class SceneInspector: public Panel, public BasicSceneInspector {
	public:
		SceneInspector();
		
	private:
		void display() override;
		void displayHierachyLevel(std::span<bloom::EntityID const>);

		
	private:
		
	};
	
	
}

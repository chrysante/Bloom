#pragma once

#include "Panel.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Graphics/Lights.hpp"

namespace poppy {
	
	class SelectionContext;
	
	class EntityInspector: public Panel {
	public:
		EntityInspector(bloom::Scene*, SelectionContext*);
		
	private:
		void display() override;
		
		void inspectTransform(bloom::EntityID);
		void inspectLight(bloom::EntityID);
		void inspectPointLight(bloom::PointLight&);
		void inspectSpotLight(bloom::SpotLight&);
		
	private:
		bloom::Scene* scene;
		SelectionContext* selection;
	};
	
}

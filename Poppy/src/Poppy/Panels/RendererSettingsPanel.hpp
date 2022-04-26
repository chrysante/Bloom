#pragma once

#include "Panel.hpp"
#include "PropertiesPanel.hpp"
#include "Bloom/Graphics/Renderer.hpp"

namespace poppy {
	
	class RendererSettingsPanel: public Panel, PropertiesPanel {
	public:
		RendererSettingsPanel(bloom::Renderer*);
		
	private:
		void display() override;
		
		void shadowProperties();
		
	private:
		bloom::Renderer* renderer;
	};
	
}

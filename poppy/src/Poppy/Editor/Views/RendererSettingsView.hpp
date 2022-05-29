#pragma once

#include "Poppy/UI/View.hpp"
#include "PropertiesView.hpp"
#include "Bloom/Graphics/Renderer.hpp"

namespace poppy {
	
	class RendererSettingsView: public View, PropertiesView {
	public:
		RendererSettingsView();
		
	private:
		void frame() override;
		
		void shadowProperties();
	};
	
}

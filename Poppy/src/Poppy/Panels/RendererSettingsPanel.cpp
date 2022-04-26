#include "RendererSettingsPanel.hpp"

#include <imgui/imgui.h>

namespace poppy {
	
	RendererSettingsPanel::RendererSettingsPanel(bloom::Renderer* renderer):
		Panel("Renderer Settings"),
		renderer(renderer)
	{
		
	}
	
	
	void RendererSettingsPanel::display() {
		shadowProperties();
	}
	
	void RendererSettingsPanel::shadowProperties() {
		
		header("Shadow Properties");
		if (beginSection()) {
			beginProperty("Resolution");
			
			mtl::uint2 resolution = renderer->getShadowMapResolution();
			
			if (ImGui::BeginCombo("##-shadow-resolution", utl::format("{}", resolution.x).data())) {
				for (int i = 6; i <= 12; ++i) {
					mtl::uint2 const value = 1u << i;
					bool const selected = resolution == value;
					if (ImGui::Selectable(utl::format("{}", value.x).data(), selected)) {
						renderer->setShadowMapResolution(value);
					}
					if (selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			
			endSection();
		}
	}
	
}

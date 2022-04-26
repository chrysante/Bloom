#pragma once

#include "Panel.hpp"
#include "BasicSceneInspector.hpp"
#include "PropertiesPanel.hpp"
#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Scene/Components.hpp"
#include "Bloom/Scene/Entity.hpp"

namespace poppy {
	
	class EntityInspector:
		public Panel,
		public BasicSceneInspector,
		private PropertiesPanel
	{
	public:
		EntityInspector();
		
	private:
		void display() override;
		
		// Component Inspectors
		void inspectTag(bloom::EntityID);
		
		void inspectTransform(bloom::EntityID);
		
		void inspectMesh(bloom::EntityID);
		void recieveMeshDragDrop(bloom::EntityID);
		
		void inspectLight(bloom::EntityID);
		void inspectLightType(bloom::LightComponent&);
		void inspectLightCommon(bloom::LightCommon&, bloom::LightType);
		void inspectPointLight(bloom::PointLight&);
		void inspectSpotLight(bloom::SpotLight&);
		void inspectDirectionalLight(bloom::DirectionalLight&);
		
		
		// Helpers
		template <typename>
		void componentHeader(std::string_view name, bloom::EntityID);
		void componentHeaderEx(std::string_view name, utl::function<void()> deleter);
		
	private:
		int editingNameState = 0;
	};
	
}

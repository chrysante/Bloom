#pragma once

#include "Panel.hpp"
#include "BasicSceneInspector.hpp"
#include "PropertiesPanel.hpp"
#include "Bloom/Graphics/Lights.hpp"
#include "Bloom/Scene/Components/AllComponents.hpp"
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
		void recieveMaterialDragDrop(bloom::EntityID);
	
		void inspectLight(bloom::EntityID);
		void inspectLightType(LightType&, bloom::EntityID);
		
		void inspectLightCommon(bloom::LightCommon&, LightType type);
		void inspectPointLight(bloom::PointLight&);
		void inspectSpotLight(bloom::SpotLight&);
		void inspectDirectionalLight(bloom::DirectionalLight&);
		void inspectSkyLight(bloom::SkyLight&);
		
		void inspectScript(bloom::EntityID);
		
		// Helpers
		bool beginComponentSection(std::string_view name);
		template <typename>
		bool beginComponentSection(std::string_view name, bloom::EntityID);
		bool beginSubSection(std::string_view name);
		
		bool beginGenericSection(std::string_view name, FontWeight, FontStyle, utl::function<void()> deleter);
		
		void endSection();
		void endSubSection();
		
	private:
		int editingNameState = 0;
	};
	
}

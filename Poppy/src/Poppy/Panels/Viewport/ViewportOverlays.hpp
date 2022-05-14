#pragma once

#include "Poppy/Panels/BasicSceneInspector.hpp"

#include <mtl/mtl.hpp>

#include "Bloom/Scene/Entity.hpp"
#include "Bloom/Scene/Components/Lights.hpp"
#include "Bloom/Scene/Components/Transform.hpp"

namespace poppy {

	class Viewport;
	
    class ViewportOverlays {
    public:
		void init(Viewport*);
		void display();
		
	private:
		template <typename>
		void drawLightOverlay();
		template <typename Light>
		void drawOneLightOverlay(bloom::EntityHandle,
								 mtl::float2 positionInWindow, bool selected,
								 bloom::Transform const&,
								 Light const&);
		
		void drawPointLightIcon(mtl::float2 position, mtl::float3 color);
		void drawSpotLightIcon(mtl::float2 position, mtl::float3 color);
		void drawDirectionalLightIcon(mtl::float2 position,
									  mtl::float3 directionWS,
									  mtl::float3 color);
		void drawSkyLightIcon(mtl::float2 position, mtl::float3 color);
		void drawSpotlightVizWS(bloom::EntityID entity,
								float radius, float angle, mtl::float3 color);
		
	private:
		Viewport* viewport = nullptr;
    };

}

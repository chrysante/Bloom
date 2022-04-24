#pragma once

#include "Panel.hpp"
#include "BasicSceneInspector.hpp"
#include "ViewportCameraActor.hpp"

#include "Bloom/Application/Event.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components.hpp"

#include "Bloom/Graphics/Renderer.hpp"
#include "Bloom/Graphics/FrameBuffer.hpp"

#include <mtl/mtl.hpp>

namespace poppy {
	
	enum class GizmoMode {
		translate = 0,
		rotate    = 1,
		scale     = 2
	};
	std::string_view toString(GizmoMode);
	
	enum class CoordinateSpace {
		world = 0,
		local = 1
	};
	std::string_view toString(CoordinateSpace);
	
	enum class Projection {
		perspective = 0, orthogonal
	};
	std::string_view toString(Projection);
	
	class Viewport: public Panel, public BasicSceneInspector {
	public:
		struct Parameters {
			float fieldOfView = 60;
		};
		
	public:
		Viewport(bloom::Renderer*);
		~Viewport();
		
		void onEvent(bloom::Event const& event) override;
		
	private:
		void init() override;
		void shutdown() override;
		void display() override;
	
		void calculateTransforms();
		void renderScene();
		void updateRenderTarget(mtl::usize2 size);
		
		void displayControls();
		
		void displayGizmo(bloom::EntityID);
		mtl::float4x4 getParentTransform(bloom::EntityID) const;
		
		void drawOverlays();
		void drawLightOverlays();
		
		void drawPointLightIcon(mtl::float2 position, mtl::float3 color);
		void drawSpotLightIcon(mtl::float2 position, mtl::float3 color);
		void drawSpotlightVizWS(mtl::float3 position, mtl::quaternion_float orientation,
								float radius, float angle, mtl::float3 color);
		
		bloom::EntityID readEntityID(mtl::float2 mousePositionInView);
		
		mtl::float3 worldSpaceToViewSpace(mtl::float3 position);
		mtl::float3 worldSpaceToWindowSpace(mtl::float3 position);
		
		
		
	private:
		bloom::Renderer* renderer = nullptr;
		bloom::EditorFrameBuffer frameBuffer;
		
		Parameters params;
		
		ViewportCameraActor cameraActor;
		Projection cameraProjection = Projection::perspective;
		
		bloom::DebugDrawMode drawMode = bloom::DebugDrawMode::lit;
		
		bool viewportHovered = false;
		
		// gizmo stuff
		void *imguizmoContext = nullptr;
		GizmoMode gizmoMode = GizmoMode::translate;
		CoordinateSpace gizmoSpace = CoordinateSpace::world;
		bool gizmoHovered = false;
	};
	
}

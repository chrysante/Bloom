#pragma once

#include "Poppy/Panels/Panel.hpp"
#include "Poppy/Panels/BasicSceneInspector.hpp"

#include "Gizmo.hpp"
#include "ViewportOverlays.hpp"
#include "ViewportCameraActor.hpp"

#include "Bloom/Application/Event.hpp"

#include "Bloom/Scene/Scene.hpp"

#include "Bloom/Graphics/Renderer.hpp"
#include "Bloom/Graphics/FrameBuffer.hpp"

#include <mtl/mtl.hpp>

namespace poppy {
	
	enum class Projection {
		perspective = 0, orthogonal
	};
	std::string_view toString(Projection);
	
	class Viewport: public Panel, public BasicSceneInspector {
	public:
		struct Parameters {
			float fieldOfView = 60;
			float nearClip = 1;
		};
		
	public:
		Viewport(bloom::Renderer*);
		
		void onEvent(bloom::Event& event) override;
				
		mtl::float3 worldSpaceToViewSpace(mtl::float3 position);
		mtl::float3 worldSpaceToWindowSpace(mtl::float3 position);
		
		mtl::float2 worldSpaceDirToViewSpace(mtl::float3 direction);
		mtl::float2 worldSpaceDirToWindowSpace(mtl::float3 direction);
		
	private:
		void init() override;
		void shutdown() override;
		void display() override;
	
		void drawScene(bloom::Renderer&);
		void updateRenderTarget(bloom::Renderer&, mtl::usize2 size);
		
		void displayControls();
		
		bloom::EntityID readEntityID(mtl::float2 mousePositionInView);
		
		void recieveSceneDragDrop();
		
	private:
		bloom::Renderer* renderer = nullptr;
		bloom::EditorFrameBuffer frameBuffer;
		
		Parameters params;
		
		ViewportCameraActor cameraActor;
		Projection cameraProjection = Projection::perspective;
		
		bloom::DebugDrawOptions drawOptions{};
		
		Gizmo gizmo;
		ViewportOverlays overlays;
		
		bool viewportHovered = false;
		bool gameView = false;
	};
	
}

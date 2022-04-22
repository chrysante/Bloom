#pragma once

#include "Panel.hpp"

#include "Bloom/Scene/Scene.hpp"
#include "Bloom/Scene/Components.hpp"

#include "Bloom/Application/Input.hpp"
#include "Bloom/Core/Time.hpp"

#include "Bloom/Graphics/Renderer.hpp"
#include "Bloom/Graphics/FrameBuffer.hpp"
#include "Bloom/Graphics/Camera.hpp"

#include <mtl/mtl.hpp>

namespace poppy {
	
	class SelectionContext;
	
	struct ViewportCameraActor {
		ViewportCameraActor() {
			camera.setTransform(position, front());
		}
		void update(bloom::TimeStep, bloom::Input const&);
		
		void applyTransform();
		
		bloom::Camera camera;
		float angleLR = mtl::constants<>::pi / 2;
		float angleUD = mtl::constants<>::pi / 2;
		float speed = 500;
		mtl::float3 position = { 0, -5, 1 };
		mtl::float3 front() const;
		mtl::float3 up() const { return { 0, 0, 1 }; }
	};
	
	enum class GizmoMode {
		translate = 0, rotate, scale
	};
	std::string_view toString(GizmoMode);
	
	enum class CoordinateSpace {
		world = 0, local
	};
	std::string_view toString(CoordinateSpace);
	
	enum class Projection {
		perspective = 0, orthogonal
	};
	std::string_view toString(Projection);
	
	class Viewport: public Panel {
	public:
		Viewport(SelectionContext*, bloom::Scene*, bloom::Renderer*);
		~Viewport();
		
		void onEvent(bloom::Event const& event) override;
		
	private:
		void init() override;
		void shutdown() override;
		void display() override;
		
		void displayControls();
		void displayGizmo(bloom::TransformComponent& transform);
		
		void renderScene();
		void updateRenderTarget(mtl::usize2 size);
		
		void drawOverlays();
		void drawLightOverlays();
		
		bloom::EntityID readEntityID(mtl::float2 mousePositionInView);
		
		mtl::float3 worldSpaceToViewSpace(mtl::float3 position);
		
	private:
		SelectionContext* selection;
		bloom::Scene* scene = nullptr;
		bloom::Renderer* renderer = nullptr;
		bloom::EditorFrameBuffer frameBuffer;
		
		ViewportCameraActor cameraActor;
		Projection cameraProjection = Projection::perspective;
		float fieldOfView = 90;
		
		bloom::DebugDrawMode drawMode = bloom::DebugDrawMode::lit;
		
		bool viewportHovered = false;
		
		// gizmo stuff
		void *imguizmoContext = nullptr;
		GizmoMode gizmoMode = GizmoMode::translate;
		CoordinateSpace gizmoSpace = CoordinateSpace::world;
		bool gizmoHovered = false;
	};
	
}

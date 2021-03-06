#pragma once

#include "Gizmo.hpp"
#include "ViewportOverlays.hpp"
#include "ViewportCameraController.hpp"

#include "Poppy/UI/View.hpp"
#include "Poppy/UI/Toolbar.hpp"
#include "Poppy/UI/PropertiesView.hpp"
#include "Poppy/Editor/Views/BasicSceneInspector.hpp"

#include "Poppy/Renderer/EditorSceneRenderer.hpp"

#include "Bloom/Core/Serialize.hpp"

#include <mtl/mtl.hpp>

namespace poppy {
	
	
	
	class Viewport: public View, BasicSceneInspector {
	public:
		Viewport();
				
		mtl::float3 worldSpaceToViewSpace(mtl::float3 position);
		mtl::float3 worldSpaceToWindowSpace(mtl::float3 position);
		
		mtl::float2 worldSpaceDirToViewSpace(mtl::float3 direction);
		mtl::float2 worldSpaceDirToWindowSpace(mtl::float3 direction);
		
//	private:
		void init() override;
		void shutdown() override;
		void frame() override;
		
		YAML::Node serialize() const override;
		void deserialize(YAML::Node) override;
		
		void onInput(bloom::InputEvent&) override;
		
		void* selectImage() const;
		void displayScene();
		void drawScene();
		void updateFramebuffer();
		
		void dropdownMenu();
		
		void debugPanel();
		
		bloom::EntityHandle readEntityID(mtl::float2 mousePositionInView);
		
		void recieveSceneDragDrop();
	
//	public:
		struct Parameters {
			enum class FramebufferElements {
				depth,
				raw,
				postprocessed,
				_count
			} framebufferSlot = FramebufferElements::postprocessed;
		};
		
		struct DebugDrawData {
			enum class Mode {
				lit = 0, wireframe, _count
			};
			Mode mode = Mode::lit;
		
			bool visualizeShadowCascades = false;
			bloom::EntityHandle lightVizEntity;
		};
		
//	private:
		Toolbar toolbar;
		ViewportOverlays overlays;
		Gizmo gizmo;
		
		Parameters params;
		
		ViewportCameraController camera;
		
		std::shared_ptr<bloom::Framebuffer> framebuffer;
		std::unique_ptr<EditorFramebuffer> editorFramebuffer;
		
		EditorSceneRenderer sceneRenderer;
		
		bool viewportHovered = false;
		bool gameView = false;
	};
	
	inline std::string toString(Viewport::DebugDrawData::Mode mode) {
		return std::array{"Lit", "Wireframe"}[(std::size_t)mode];
	};
	
}

BLOOM_MAKE_TEXT_SERIALIZER(poppy::Viewport::Parameters,
						   framebufferSlot);

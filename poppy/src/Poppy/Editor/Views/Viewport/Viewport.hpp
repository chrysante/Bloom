#pragma once

#include <mtl/mtl.hpp>

#include "Bloom/Core/Serialize.hpp"
#include "Poppy/Editor/Views/BasicSceneInspector.hpp"
#include "Poppy/Editor/Views/Viewport/Gizmo.hpp"
#include "Poppy/Editor/Views/Viewport/ViewportCameraController.hpp"
#include "Poppy/Editor/Views/Viewport/ViewportOverlays.hpp"
#include "Poppy/Renderer/EditorSceneRenderer.hpp"
#include "Poppy/UI/PropertiesView.hpp"
#include "Poppy/UI/Toolbar.hpp"
#include "Poppy/UI/View.hpp"

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

    void receiveSceneDragDrop();

    //	public:
    struct Parameters {
        enum class FramebufferElements {
            depth,
            raw,
            postprocessed,
            _count
        } framebufferSlot = FramebufferElements::postprocessed;
    };

    struct DebugDrawOptions {
        enum class Mode { lit = 0, wireframe, _count };
        Mode mode = Mode::lit;

        bool visualizeShadowCascades = false;
        bloom::EntityHandle lightVizEntity;
    };

    //	private:
    Toolbar toolbar;
    ViewportOverlays overlays;
    Gizmo gizmo;

    Parameters params;
    DebugDrawOptions drawOptions;

    ViewportCameraController camera;

    std::shared_ptr<bloom::Framebuffer> framebuffer;
    std::unique_ptr<EditorFramebuffer> editorFramebuffer;

    EditorSceneRenderer sceneRenderer;

    bool viewportHovered = false;
    bool gameView = false;
};

inline std::string toString(Viewport::DebugDrawOptions::Mode mode) {
    return std::array{ "Lit", "Wireframe" }[(std::size_t)mode];
};

} // namespace poppy

BLOOM_MAKE_TEXT_SERIALIZER(poppy::Viewport::Parameters, framebufferSlot);

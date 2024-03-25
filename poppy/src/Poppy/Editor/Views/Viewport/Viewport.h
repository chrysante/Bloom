#ifndef POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORT_H
#define POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORT_H

#include <vml/vml.hpp>

#include "Bloom/Core/Serialize.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/Editor/Views/Viewport/Gizmo.h"
#include "Poppy/Editor/Views/Viewport/ViewportCameraController.h"
#include "Poppy/Editor/Views/Viewport/ViewportOverlays.h"
#include "Poppy/Renderer/EditorSceneRenderer.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"

namespace poppy {

class Viewport: public View, BasicSceneInspector {
public:
    Viewport();

    vml::float3 worldSpaceToViewSpace(vml::float3 position);
    vml::float3 worldSpaceToWindowSpace(vml::float3 position);

    vml::float2 worldSpaceDirToViewSpace(vml::float3 direction);
    vml::float2 worldSpaceDirToWindowSpace(vml::float3 direction);

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

    bloom::EntityHandle readEntityID(vml::float2 mousePositionInView);

    void receiveSceneDragDrop();

    //	public:
    struct Parameters {
        enum class FramebufferElements {
            Depth,
            Raw,
            Postprocessed,
            LAST = Postprocessed
        };
        FramebufferElements framebufferSlot =
            FramebufferElements::Postprocessed;
    };

    struct DebugDrawOptions {
        enum class Mode { Lit = 0, Wireframe, LAST = Wireframe };
        Mode mode = Mode::Lit;

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

#endif // POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORT_H

#ifndef POPPY_RENDERER_EDITORSCENERENDERER_H
#define POPPY_RENDERER_EDITORSCENERENDERER_H

#include "Bloom/Graphics/Renderer/SceneRenderer.h"
#include "Poppy/Renderer/EditorDrawData.h"
#include "Poppy/Renderer/EditorSceneRenderer.h"

namespace poppy {

class EditorRenderer;
class EditorFramebuffer;
class SelectionContext;

class EditorSceneRenderer: public bloom::SceneRenderer {
public:
    using bloom::SceneRenderer::SceneRenderer;

    void drawOverlays(SelectionContext const&, OverlayDrawDescription const&,
                      bloom::Framebuffer&, EditorFramebuffer&,
                      bloom::CommandQueue&);

private:
    void submitExtra() override;

private:
    SelectionContext const* mSelection = nullptr;
    OverlayDrawDescription mDrawDesc;
};

} // namespace poppy

#endif // POPPY_RENDERER_EDITORSCENERENDERER_H

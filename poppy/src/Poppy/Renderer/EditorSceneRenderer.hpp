#pragma once

#include "Bloom/Graphics/Renderer/SceneRenderer.hpp"
#include "Poppy/Renderer/EditorDrawData.hpp"
#include "Poppy/Renderer/EditorSceneRenderer.hpp"

namespace poppy {

class EditorRenderer;
class EditorFramebuffer;
class SelectionContext;

class EditorSceneRenderer: public bloom::SceneRenderer {
public:
    using bloom::SceneRenderer::SceneRenderer;

    void drawWithOverlays(std::span<bloom::Scene const* const>,
                          SelectionContext const&, bloom::Camera const&,
                          OverlayDrawDescription const&, bloom::Framebuffer&,
                          EditorFramebuffer&, bloom::CommandQueue&);

private:
    void submitExtra() override;

private:
    SelectionContext const* mSelection = nullptr;
    OverlayDrawDescription mDrawDesc;
};

} // namespace poppy

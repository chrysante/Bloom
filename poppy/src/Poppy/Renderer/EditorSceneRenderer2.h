#ifndef POPPY_RENDERER_EDITORSCENERENDERER2_H
#define POPPY_RENDERER_EDITORSCENERENDERER2_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/SceneRenderer.h"
#include "Poppy/Editor/EntitySelectionManager.h"

namespace poppy {

class EditorSceneRenderer2 {
public:
    explicit EditorSceneRenderer2(bloom::rg::RenderGraph* renderGraph);
    void begin(bloom::rg::FrameParameters const& frameParams,
               bloom::Camera const& camera);
    void submit(bloom::Scene const& scene,
                EntitySelectionManager const& selectionManager);
    void draw(bloom::CommandQueue& commandQueue);

    bloom::TextureView sceneColor() const;
    bloom::TextureView sceneColorWithOverlays() const;

private:
    std::unique_ptr<bloom::rg::SceneRenderer> mRenderer;
    std::unique_ptr<bloom::rg::RenderGraph> mRenderGraph;
    bloom::rg::GeometryResource* selectedGeometry = nullptr;
    bloom::rg::BufferResource* cameraBuffer = nullptr;
    bloom::rg::TextureResource* mCompositeColor = nullptr;
    bloom::rg::TextureResource* mSceneColor = nullptr;
};

} // namespace poppy

#endif // POPPY_RENDERER_EDITORSCENERENDERER2_H

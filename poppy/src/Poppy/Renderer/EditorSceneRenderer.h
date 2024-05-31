#ifndef POPPY_RENDERER_EDITORSCENERENDERER_H
#define POPPY_RENDERER_EDITORSCENERENDERER_H

#include <memory>
#include <vector>

#include <vml/vml.hpp>

#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Renderer.h"
#include "Poppy/Editor/EntitySelectionManager.h"

namespace poppy {

class EditorSceneRenderer {
public:
    explicit EditorSceneRenderer(
        std::unique_ptr<bloom::RenderGraph> renderGraph,
        bloom::CommandQueue& commandQueue);
    void begin(bloom::FrameParameters const& frameParams,
               bloom::Camera const& camera, bloom::BloomSettings bloomSettings,
               bloom::PostProcessSettings postProcessSettings);
    void submit(bloom::Scene const& scene,
                EntitySelectionManager const& selectionManager);
    void draw(bloom::CommandBuffer& commandBuffer);

    bloom::TextureView sceneColor() const;
    bloom::TextureView sceneColorWithOverlays() const;

    bloom::EntityID readEntityIDMap(bloom::CommandQueue& commandQueue,
                                    vml::float2 position) const;

    bloom::RenderGraph* renderGraph() { return mRenderGraph.get(); }
    bloom::RenderGraph* sceneRenderGraph() { return mSceneRenderGraph; }

private:
    std::unique_ptr<bloom::Renderer> mRenderer;
    std::unique_ptr<bloom::RenderGraph> mRenderGraph;
    bloom::RenderGraph* mSceneRenderGraph = nullptr;
    bloom::GeometryResource* selectedGeometry = nullptr;
    bloom::Geometry2DResource* overlayScene = nullptr;
    bloom::TextureResource* idMap = nullptr;
    bloom::TextureResource* mCompositeColor = nullptr;
    bloom::TextureResource* mSceneColor = nullptr;
};

} // namespace poppy

#endif // POPPY_RENDERER_EDITORSCENERENDERER_H

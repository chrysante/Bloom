#ifndef POPPY_RENDERER_EDITORSCENERENDERER_H
#define POPPY_RENDERER_EDITORSCENERENDERER_H

#include <memory>
#include <span>
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
               bloom::SceneParameters const& sceneParams);
    void submit(bloom::Scene const& scene);
    void submitSelection(std::span<bloom::EntityHandle const> selection);
    void draw(bloom::CommandBuffer& commandBuffer);

    bloom::TextureHandle sceneColor() const;
    bloom::TextureHandle sceneColorWithOverlays() const;

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

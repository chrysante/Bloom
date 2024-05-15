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
        std::unique_ptr<bloom::RenderGraph> renderGraph);
    void begin(bloom::FrameParameters const& frameParams,
               bloom::Camera const& camera);
    void submit(bloom::Scene const& scene,
                EntitySelectionManager const& selectionManager);
    void setIBLTexture(bloom::Reference<bloom::Texture2D> tex);
    void draw(bloom::CommandQueue& commandQueue);

    bloom::TextureView sceneColor() const;
    bloom::TextureView sceneColorWithOverlays() const;

    bloom::EntityHandle readEntityIDMap(bloom::CommandQueue& commandQueue,
                                        std::span<bloom::Scene* const> scenes,
                                        vml::float2 position) const;

    bloom::RenderGraph* renderGraph() { return mRenderGraph.get(); }

private:
    std::unique_ptr<bloom::Renderer> mRenderer;
    std::unique_ptr<bloom::RenderGraph> mRenderGraph;
    bloom::GeometryResource* selectedGeometry = nullptr;
    bloom::TextureResource* idMap = nullptr;
    bloom::TextureResource* mCompositeColor = nullptr;
    bloom::TextureResource* mSceneColor = nullptr;
};

} // namespace poppy

#endif // POPPY_RENDERER_EDITORSCENERENDERER_H

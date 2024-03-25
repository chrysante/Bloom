#ifndef POPPY_RENDERER_EDITORRENDERER_H
#define POPPY_RENDERER_EDITORRENDERER_H

#include <memory>

#include <utl/structure_of_arrays.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Renderer.h"
#include "Poppy/Renderer/EditorDrawData.h"

namespace poppy {

struct ObjectEditorRenderData {
    bool selected = false;
};

struct EditorFramebuffer: bloom::Framebuffer {
    bloom::TextureHandle selected;
    bloom::TextureHandle selectedDepth;
    bloom::TextureHandle composed;
};

namespace {
UTL_SOA_TYPE(RenderObjectData,
             (bloom::Reference<bloom::StaticMeshRenderer>, mesh),
             (vml::float4x4, transform));
}

class EditorRenderer: public bloom::Renderer {
public:
    /// MARK: Framebuffer Creation
    std::unique_ptr<bloom::Framebuffer> createFramebuffer(
        vml::int2 size) const override;
    std::unique_ptr<bloom::Framebuffer> createDebugFramebuffer(
        vml::int2 size) const override;
    std::unique_ptr<EditorFramebuffer> createEditorFramebuffer(
        vml::int2 size) const;

    /// MARK: Initialization
    explicit EditorRenderer(bloom::Receiver, std::shared_ptr<bloom::Renderer>);
    void init(bloom::HardwareDevice&) override;

    /// MARK: Scene Construction
    void beginScene(bloom::Camera const&) override;
    void endScene() override;

    void submit(bloom::Reference<bloom::StaticMeshRenderer>,
                bloom::Reference<bloom::MaterialInstance>,
                vml::float4x4 const& transform) override;
    void submitSelected(bloom::Reference<bloom::StaticMeshRenderer>,
                        vml::float4x4 const& transform);
    void submit(bloom::PointLight const&) override;
    void submit(bloom::SpotLight const&) override;
    void submit(bloom::DirectionalLight const&) override;
    void submit(bloom::SkyLight const&) override;

    void submitShadowCascadeViz(bloom::DirectionalLight const&);

    /// MARK: Draw
    void draw(bloom::Framebuffer&, bloom::CommandQueue&) override;
    void drawOverlays(bloom::Framebuffer&, EditorFramebuffer&,
                      bloom::CommandQueue&, OverlayDrawDescription const&);

    Renderer& wrappedRenderer() const { return *mRenderer; }

private:
    void selectedObjectsPass(EditorFramebuffer&, bloom::CommandQueue&);
    void compositionPass(bloom::Framebuffer&, EditorFramebuffer&,
                         bloom::CommandQueue&, OverlayDrawDescription const&);

private:
    std::shared_ptr<bloom::Renderer> mRenderer;
    utl::structure_of_arrays<RenderObjectData> selectedObjects;
    bloom::BufferHandle editorDrawDataBuffer;
    bloom::RenderPipelineHandle compositionPipeline;
    bloom::BufferHandle selectedTransformsBuffer;
    bloom::RenderPipelineHandle selectedPipeline;
    bloom::DepthStencilHandle depthStencil;
};

} // namespace poppy

#endif // POPPY_RENDERER_EDITORRENDERER_H

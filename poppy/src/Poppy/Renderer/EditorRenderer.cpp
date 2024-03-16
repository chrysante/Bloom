#include "Poppy/Renderer/EditorRenderer.h"

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Renderer/ForwardRenderer.h"
#include "Bloom/Graphics/StaticMesh.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

/// MARK: Framebuffer Creation
std::unique_ptr<bloom::Framebuffer> EditorRenderer::createFramebuffer(
    mtl::int2 size) const {
    return mRenderer->createFramebuffer(size);
}

std::unique_ptr<bloom::Framebuffer> EditorRenderer::createDebugFramebuffer(
    mtl::int2 size) const {
    return mRenderer->createDebugFramebuffer(size);
}

std::unique_ptr<EditorFramebuffer> EditorRenderer::createEditorFramebuffer(
    mtl::int2 size) const {
    auto framebuffer = std::make_unique<EditorFramebuffer>();

    TextureDescription desc;
    desc.size = { size, 1 };
    desc.usage = TextureUsage::ShaderRead | TextureUsage::RenderTarget;
    desc.storageMode = StorageMode::GPUOnly;
    desc.type = TextureType::Texture2D;

    desc.pixelFormat = PixelFormat::R8Unorm;
    framebuffer->selected = device().createTexture(desc);

    desc.pixelFormat = PixelFormat::Depth32Float;
    framebuffer->selectedDepth = device().createTexture(desc);

    desc.pixelFormat = PixelFormat::RGBA8Unorm;
    framebuffer->composed = device().createTexture(desc);

    framebuffer->size = size;
    return framebuffer;
}

/// MARK: Initialization
EditorRenderer::EditorRenderer(bloom::Receiver receiver,
                               std::shared_ptr<Renderer> renderer):
    Renderer(std::move(receiver)), mRenderer(std::move(renderer)) {}

void EditorRenderer::init(HardwareDevice& device) {
    mDevice = &device;

    /* Create Depth Stencil */ {
        DepthStencilDescription depthDesc;
        depthDesc.depthCompareFunction = CompareFunction::LessEqual;
        depthDesc.depthWrite = true;
        depthStencil = device.createDepthStencil(depthDesc);
    }

    /* Create Selection Pipeline */ {
        RenderPipelineDescription desc;
        ColorAttachmentDescription caDesc;
        caDesc.pixelFormat = PixelFormat::R8Unorm;
        desc.colorAttachments.push_back(caDesc);
        desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;

        desc.vertexFunction = device.createFunction("selectionPassVS");
        desc.fragmentFunction = device.createFunction("selectionPassFS");

        selectedPipeline = device.createRenderPipeline(desc);
    }

    /* Create Composition Pipeline */ {
        RenderPipelineDescription desc;
        ColorAttachmentDescription caDesc;
        caDesc.pixelFormat = PixelFormat::RGBA8Unorm;
        desc.colorAttachments.push_back(caDesc);

        desc.vertexFunction = device.createFunction("postprocessVS");
        desc.fragmentFunction = device.createFunction("editorCompositionFS");

        compositionPipeline = device.createRenderPipeline(desc);
    }

    /* Create Editor Draw Data Buffer */ {
        BufferDescription desc;
        desc.size = sizeof(EditorDrawData);
        desc.storageMode = StorageMode::Managed;
        editorDrawDataBuffer = device.createBuffer(desc);
    }
}

/// MARK: Scene Construction
void EditorRenderer::beginScene(Camera const& camera) {
    mRenderer->beginScene(camera);
    selectedObjects.clear();
}

static auto const selectedObjectsOrder = [](auto&& a, auto&& b) {
    return a.mesh < b.mesh;
};

void EditorRenderer::endScene() { mRenderer->endScene(); }

void EditorRenderer::submit(Reference<StaticMeshRenderer> mesh,
                            Reference<MaterialInstance> material,
                            mtl::float4x4 const& transform) {
    mRenderer->submit(std::move(mesh), std::move(material), transform);
}

void EditorRenderer::submitSelected(Reference<StaticMeshRenderer> mesh,
                                    mtl::float4x4 const& transform) {
    selectedObjects.push_back({ mesh, mtl::transpose(transform) });
}

void EditorRenderer::submit(PointLight const& light) {
    mRenderer->submit(light);
}

void EditorRenderer::submit(SpotLight const& light) {
    mRenderer->submit(light);
}

void EditorRenderer::submit(DirectionalLight const& light) {
    mRenderer->submit(light);
}

void EditorRenderer::submit(SkyLight const& light) { mRenderer->submit(light); }

void EditorRenderer::submitShadowCascadeViz(DirectionalLight const&) {
    // ...
}

/// MARK: Draw
void EditorRenderer::draw(Framebuffer& framebuffer,
                          CommandQueue& commandQueue) {
    mRenderer->draw(framebuffer, commandQueue);
}

void EditorRenderer::drawOverlays(Framebuffer& framebuffer,
                                  EditorFramebuffer& editorFramebuffer,
                                  CommandQueue& commandQueue,
                                  OverlayDrawDescription const& drawDesc) {
    if (drawDesc.drawSelection) {
        selectedObjectsPass(editorFramebuffer, commandQueue);
    }

    compositionPass(framebuffer, editorFramebuffer, commandQueue, drawDesc);
}

void EditorRenderer::selectedObjectsPass(EditorFramebuffer& framebuffer,
                                         bloom::CommandQueue& commandQueue) {
    std::sort(selectedObjects.begin(), selectedObjects.end(),
              selectedObjectsOrder);

    /* Upload transforms of selected entities */ {
        std::size_t const targetBufferSize =
            selectedObjects.size() * sizeof(float4x4);
        if (selectedTransformsBuffer.size() < targetBufferSize) {
            BufferDescription desc;
            desc.size = targetBufferSize;
            desc.storageMode = StorageMode::Managed;
            selectedTransformsBuffer = device().createBuffer(desc);
        }
        device().fillManagedBuffer(selectedTransformsBuffer,
                                   selectedObjects.data().transform,
                                   targetBufferSize);
    }

    std::unique_ptr _ctx = commandQueue.createRenderContext();
    auto& ctx = *_ctx;

    RenderPassDescription desc{};
    RenderPassColorAttachmentDescription caDesc{};
    caDesc.texture = framebuffer.selected;
    caDesc.clearColor = { 0, 0, 0, 1 };
    caDesc.loadAction = LoadAction::Clear;
    desc.colorAttachments.push_back(caDesc);

    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = framebuffer.selectedDepth;

    desc.depthAttachment = dDesc;
    ctx.begin(desc);

    auto& renderer = utl::down_cast<ForwardRenderer&>(*mRenderer);

    // Vertex buffers
    ctx.setVertexBuffer(renderer.renderObjects.parameterBuffer, 0,
                        offsetof(RendererParameters, scene));
    ctx.setVertexBuffer(selectedTransformsBuffer, 2);

    ctx.setPipeline(selectedPipeline);
    ctx.setTriangleCullMode(TriangleCullMode::Back);
    ctx.setDepthStencil(depthStencil);

    StaticMeshRenderer* currentMesh = nullptr;
    for (size_t index = 0; auto&& object: selectedObjects) {
        if (object.mesh.get() != currentMesh) {
            ctx.setVertexBuffer(object.mesh->vertexBuffer(), 1);
        }
        // object transform
        ctx.setVertexBufferOffset(2, index * sizeof(float4x4));

        BufferView indexBuffer = object.mesh->indexBuffer();

        DrawDescription desc{};
        desc.indexCount = indexBuffer.size() / 4;
        desc.indexType = IndexType::U32;
        desc.indexBuffer = indexBuffer;
        ctx.draw(desc);
        ++index;
    }
    ctx.end();

    ctx.commit();
}

void EditorRenderer::compositionPass(Framebuffer& framebuffer,
                                     EditorFramebuffer& editorFramebuffer,
                                     bloom::CommandQueue& commandQueue,
                                     OverlayDrawDescription const& drawDesc) {
    /* Upload Editor Draw Data */ {
        EditorDrawData drawData;
        drawData.overlayDrawDesc = drawDesc;
        device().fillManagedBuffer(editorDrawDataBuffer, &drawData,
                                   sizeof drawData);
    }

    std::unique_ptr _ctx = commandQueue.createRenderContext();
    auto& ctx = *_ctx;

    RenderPassDescription desc{};
    RenderPassColorAttachmentDescription caDesc{};
    caDesc.texture = editorFramebuffer.composed;
    caDesc.loadAction = LoadAction::DontCare;
    desc.colorAttachments.push_back(caDesc);

    ctx.begin(desc);

    auto& renderer = utl::down_cast<ForwardRenderer&>(*mRenderer);
    auto& fwFramebuffer =
        utl::down_cast<ForwardRendererFramebuffer&>(framebuffer);

    ctx.setPipeline(compositionPipeline);

    ctx.setFragmentBuffer(renderer.renderObjects.parameterBuffer, 0,
                          offsetof(RendererParameters, scene));
    ctx.setFragmentBuffer(editorDrawDataBuffer, 1);
    ctx.setFragmentTexture(fwFramebuffer.postProcessed, 0);
    ctx.setFragmentTexture(fwFramebuffer.depth, 1);
    ctx.setFragmentTexture(editorFramebuffer.selected, 2);
    ctx.setFragmentTexture(editorFramebuffer.selectedDepth, 3);
    ctx.setFragmentSampler(renderer.renderObjects.postprocessSampler, 0);

    ctx.draw(0, 6);
    ctx.end();

    ctx.commit();
}

#include "Poppy/Renderer/EditorSceneRenderer.h"

#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/Renderer2D.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Renderer/Passes.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

template <typename R>
static R* findSink(RenderGraph* renderGraph, std::string_view name) {
    auto* source = renderGraph->findInputSource(name);
    if (!source) {
        Logger::Warn("Sink not found: ", name);
        return nullptr;
    }
    return dyncast<R*>(source->resource());
}

template <typename R>
static R* findSource(RenderGraph* renderGraph, std::string_view name) {
    auto* source = renderGraph->findSource(name);
    if (!source) {
        Logger::Warn("Resource not found: ", name);
        return nullptr;
    }
    return dyncast<R*>(source->resource());
}

static void wrapRenderGraph(RenderGraph& outer,
                            std::unique_ptr<RenderGraph> innerOwner) {
    auto& inner = *outer.addPass(std::move(innerOwner));
    for (auto* sink: inner.sinks()) {
        auto* outerSink = outer.declareSink(sink->interface());
        outer.addLink(outerSink->internalSource(), sink);
    }
    for (auto* source: inner.sources()) {
        auto* outerSource = outer.declareSource(source->interface());
        outer.addLink(source, outerSource->internalSink());
    }
}

EditorSceneRenderer::EditorSceneRenderer(
    std::unique_ptr<RenderGraph> sceneRenderGraphOwner,
    CommandQueue& commandQueue) {
    auto* sceneRenderGraph = sceneRenderGraphOwner.get();
    mRenderGraph = std::make_unique<RenderGraph>("Editor Render Graph",
                                                 sceneRenderGraph->device());
    wrapRenderGraph(*mRenderGraph, std::move(sceneRenderGraphOwner));
    auto* selPass = mRenderGraph->addPass(makeSelectionPass());
    auto* overlaysPass = mRenderGraph->addPass(makeOverlayPass());
    auto* compPass = mRenderGraph->addPass(makeCompositionPass());

    auto* selGeo = mRenderGraph->declareSink(
        GeometryInterface{ "Selected Geometry", nullptr });
    auto* overlays = mRenderGraph->declareSink(
        Geometry2DInterface{ "Debug Overlays", nullptr });
    auto* camera = mRenderGraph->findInputSource("Camera");
    auto* compositeColor = mRenderGraph->declareSource(
        TextureInterface{ "Composite Color", nullptr, {} });
    auto* idMapSource = mRenderGraph->findSource("ID Map");
    auto* sceneColor = sceneRenderGraph->findSource("Color");
    // Link selection pass
    mRenderGraph->addLink(selGeo->internalSource(),
                          selPass->findSink("Selected Geometry"));
    mRenderGraph->addLink(camera, selPass->findSink("Camera"));
    // Link overlays pass
    mRenderGraph->addLink(sceneRenderGraph->findSource("Depth"),
                          overlaysPass->findSink("Depth"));
    mRenderGraph->addLink(sceneRenderGraph->findSource("Color"),
                          overlaysPass->findSink("Color"));
    mRenderGraph->addLink(camera, overlaysPass->findSink("Camera"));
    mRenderGraph->addLink(overlays->internalSource(),
                          overlaysPass->findSink("Geometry"));
    // Link composition pass
    mRenderGraph->addLink(selPass->findSource("Selection Map"),
                          compPass->findSink("Selection Map"));
    mRenderGraph->addLink(selPass->findSource("Selection Depth"),
                          compPass->findSink("Selection Depth"));
    mRenderGraph->addLink(overlaysPass->findSource("Color"),
                          compPass->findSink("Scene Color"));
    mRenderGraph->addLink(sceneRenderGraph->findSource("Depth"),
                          compPass->findSink("Scene Depth"));
    // Link internal render graph sinks
    mRenderGraph->addLink(compPass->findSource("Final Color"),
                          compositeColor->internalSink());
    if (!mRenderGraph->compile()) {
        return;
    }
    mRenderGraph->allocateBackingResources();
    selectedGeometry = dyncast<GeometryResource*>(selGeo->resource());
    this->overlayScene = dyncast<Geometry2DResource*>(overlays->resource());
    idMap = dyncast<TextureResource*>(idMapSource->resource());
    mCompositeColor = dyncast<TextureResource*>(compositeColor->resource());
    mSceneColor = dyncast<TextureResource*>(sceneColor->resource());
    mRenderer = std::make_unique<Renderer>(mRenderGraph.get(), commandQueue);
}

void EditorSceneRenderer::begin(bloom::FrameParameters const& frameParams,
                                bloom::Camera const& camera) {
    if (!mRenderer) {
        return;
    }
    mRenderer->begin(frameParams, camera);
    if (selectedGeometry) {
        selectedGeometry->clear();
    }
    if (overlayScene) {
        overlayScene->scene = {};
    }
}

static void submitSelectedGeometry(
    Scene const& scene, GeometryResource& geometry,
    EntitySelectionManager const& selectionManager) {
    auto view = scene.view<TransformMatrixComponent const,
                           MeshRendererComponent const>();
    view.each([&](EntityID ID, TransformMatrixComponent const& transform,
                  MeshRendererComponent const& meshRenderer) {
        if (!selectionManager.contains(scene.getHandle(ID))) {
            return;
        }
        if (!meshRenderer.mesh || !meshRenderer.mesh->getRenderer() ||
            !meshRenderer.materialInstance ||
            !meshRenderer.materialInstance->material())
        {
            return;
        }
        auto impl = meshRenderer.mesh->getRenderer();
        if (!impl) {
            return;
        }
        geometry.add(std::move(impl), meshRenderer.materialInstance,
                     transform.matrix, 0);
    });
}

static void addSphereVis(Renderer2DScene& overlay, float3 position,
                         float radius, float4 color, float width) {
    overlay.addCircle(position, { 1, 0, 0 }, radius, color, width, 64);
    overlay.addCircle(position, { 0, 1, 0 }, radius, color, width, 64);
    overlay.addCircle(position, { 0, 0, 1 }, radius, color, width, 64);
}

static float pointLightInfluence(PointLight l,
                                 float minAttenuation = 2.0 / 256) {
    float tmp = (l.intensity / minAttenuation - l.radius - 1) / l.falloff;
    return std::sqrt(tmp);
}

static void submitPointlightOverlay(ConstEntityHandle entity,
                                    Renderer2DScene& overlayScene) {
    auto* light = entity.tryGet<PointLightComponent>();
    auto* transform = entity.tryGet<TransformMatrixComponent>();
    if (!light || !transform) {
        return;
    }
    float3 pos = transform->matrix.column(3).xyz;
    float4 color(light->color, 1);
    float influence = pointLightInfluence(*light);
    addSphereVis(overlayScene, pos, influence, color, 6);
    addSphereVis(overlayScene, pos, light->radius, color, 6);
}

void EditorSceneRenderer::submit(
    bloom::Scene const& scene, EntitySelectionManager const& selectionManager) {
    if (!mRenderer) {
        return;
    }
    mRenderer->submit(scene);
    if (selectedGeometry) {
        submitSelectedGeometry(scene, *selectedGeometry, selectionManager);
    }
    if (overlayScene) {
        for (auto& entity: selectionManager) {
            submitPointlightOverlay(entity, overlayScene->scene);
        }
    }
}

void EditorSceneRenderer::setIBLTexture(Reference<Texture2D> tex) {
    if (mRenderer) {
        mRenderer->setIBLTexture(std::move(tex));
    }
}

void EditorSceneRenderer::draw(bloom::CommandBuffer& commandBuffer) {
    if (!mRenderer) {
        return;
    }
    mRenderGraph->allocateResources(mRenderer->getFrameParams(),
                                    mRenderGraph->device());
    if (selectedGeometry) {
        selectedGeometry->prepare(mRenderGraph->device());
    }
    mRenderer->draw(commandBuffer);
}

bloom::TextureView EditorSceneRenderer::sceneColor() const {
    return mSceneColor ? mSceneColor->handle() : nullptr;
}

bloom::TextureView EditorSceneRenderer::sceneColorWithOverlays() const {
    return mCompositeColor ? mCompositeColor->handle() : nullptr;
}

bloom::EntityHandle EditorSceneRenderer::readEntityIDMap(
    CommandQueue& commandQueue, std::span<Scene* const> scenes,
    float2 position) const {
    if (!idMap) {
        return {};
    }
    auto idTex = idMap->handle();
    auto commandBuffer = commandQueue.createCommandBuffer();
    auto encoder = commandBuffer->makeBlitEncoder();
    encoder->synchronizeTexture(idTex, 0, 0);
    encoder->end();
    commandBuffer->commit();
    commandBuffer->waitForCompletion();
    EntityHandleEncoding encodedID;
    vml::AABB<uint32_t, 3> region = { uint3(position, 0),
                                      uint3(position, 0) + uint3(1) };
    copyTextureToMemory(RenderAPI::Metal, idTex, &encodedID, 4, region, 0);
    if (!encodedID.valid() || encodedID.sceneIndex >= scenes.size()) {
        return {};
    }
    auto* scene = scenes[encodedID.sceneIndex];
    return scene->getHandle(EntityID(encodedID.entityID));
}

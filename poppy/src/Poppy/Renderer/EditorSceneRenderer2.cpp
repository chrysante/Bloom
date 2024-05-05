#include "Poppy/Renderer/EditorSceneRenderer2.h"

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Renderer/Passes.h"

using namespace poppy;
using namespace bloom;
using namespace rg;

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

EditorSceneRenderer2::EditorSceneRenderer2(RenderGraph* sceneRenderGraph) {
    mRenderGraph = std::make_unique<RenderGraph>("Editor Render Graph",
                                                 sceneRenderGraph->context(),
                                                 sceneRenderGraph->device());
    auto* selPass = mRenderGraph->addPass(makeSelectionPass());
    auto* compPass = mRenderGraph->addPass(makeCompositionPass());
    auto* selGeo = mRenderGraph->declareSink(
        GeometryInterface{ "Selected Geometry", nullptr });
    auto* camera = mRenderGraph->declareSink(
        BufferInterface{ "Camera Buffer", nullptr, { sizeof(CameraInput) } });
    auto* compositeColor = mRenderGraph->declareSource(
        TextureInterface{ "Final Color", nullptr, {} });
    auto* sceneColor = sceneRenderGraph->findSource("Color");
    mRenderGraph->addLink(selGeo->internalSource(),
                          selPass->findSink("Selected Geometry"));
    mRenderGraph->addLink(camera->internalSource(),
                          selPass->findSink("Camera"));
    mRenderGraph->addLink(selPass->findSource("Selection Map"),
                          compPass->findSink("Selection Map"));
    mRenderGraph->addLink(selPass->findSource("Selection Depth"),
                          compPass->findSink("Selection Depth"));
    mRenderGraph->addLink(sceneRenderGraph->findSource("Color"),
                          compPass->findSink("Scene Color"));
    mRenderGraph->addLink(sceneRenderGraph->findSource("Depth"),
                          compPass->findSink("Scene Depth"));
    mRenderGraph->addLink(compPass->findSource("Final Color"),
                          compositeColor->internalSink());
    if (!mRenderGraph->compile()) {
        return;
    }
    mRenderGraph->allocateBackingResources();
    selectedGeometry = dyncast<GeometryResource*>(selGeo->resource());
    cameraBuffer = dyncast<BufferResource*>(camera->resource());
    mCompositeColor = dyncast<TextureResource*>(compositeColor->resource());
    mSceneColor = dyncast<TextureResource*>(sceneColor->resource());
    mRenderer = std::make_unique<rg::SceneRenderer>(sceneRenderGraph);
}

void EditorSceneRenderer2::begin(bloom::rg::FrameParameters const& frameParams,
                                 bloom::Camera const& camera) {
    mRenderer->begin(frameParams, camera);
    if (selectedGeometry) {
        selectedGeometry->clear();
    }
}

static void submitSelectedGeometry(
    Scene const& scene, rg::GeometryResource& geometry,
    EntitySelectionManager const& selectionManager) {
    auto view = scene.view<TransformMatrixComponent const,
                           MeshRendererComponent const>();
    view.each([&](EntityID ID, TransformMatrixComponent const& transform,
                  MeshRendererComponent const& meshRenderer) {
        if (!selectionManager.contains(scene.getHandle(ID))) {
            return;
        }
        if (!meshRenderer.mesh || !meshRenderer.materialInstance ||
            !meshRenderer.materialInstance->material())
        {
            return;
        }
        geometry.add(meshRenderer.mesh->getRenderer(),
                     meshRenderer.materialInstance, transform.matrix);
    });
}

void EditorSceneRenderer2::submit(
    bloom::Scene const& scene, EntitySelectionManager const& selectionManager) {
    mRenderer->submit(scene);
    if (selectedGeometry) {
        submitSelectedGeometry(scene, *selectedGeometry, selectionManager);
    }
}

void EditorSceneRenderer2::draw(bloom::CommandQueue& commandQueue) {
    mRenderer->draw(commandQueue);
    mRenderGraph->allocateResources(mRenderer->getFrameParams(),
                                    mRenderGraph->device());
    if (selectedGeometry) {
        selectedGeometry->prepare(mRenderGraph->device());
    }
    if (cameraBuffer && cameraBuffer->handle()) {
        auto camera = mRenderer->getCamera();
        CameraInput value = {
            .camera = vml::transpose(camera.viewProjection()),
            .cameraPosition = camera.position(),
        };
        mRenderGraph->device().fillManagedBuffer(cameraBuffer->handle(), &value,
                                                 sizeof value);
    }
    mRenderGraph->execute(mRenderer->getFrameParams(), commandQueue);
}

bloom::TextureView EditorSceneRenderer2::sceneColor() const {
    return mSceneColor ? mSceneColor->handle() : nullptr;
}

bloom::TextureView EditorSceneRenderer2::sceneColorWithOverlays() const {
    return mCompositeColor ? mCompositeColor->handle() : nullptr;
}

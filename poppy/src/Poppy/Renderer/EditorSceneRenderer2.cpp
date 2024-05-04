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

EditorSceneRenderer2::EditorSceneRenderer2(RenderGraph* renderGraph):
    mRenderer(renderGraph) {
    mRenderGraph = std::make_unique<RenderGraph>("Editor Render Graph",
                                                 renderGraph->context(),
                                                 renderGraph->device());
    auto* selPass = mRenderGraph->addPass(makeSelectionPass());
    auto* selGeo = mRenderGraph->declareSink(
        GeometryInterface{ "Selected Geometry", nullptr });
    auto* camera = mRenderGraph->declareSink(
        BufferInterface{ "Camera Buffer", nullptr, { sizeof(CameraInput) } });
    auto* finalColor = mRenderGraph->declareSource(
        TextureInterface{ "Final Color", nullptr, {} });
    mRenderGraph->addLink(selGeo->internalSource(),
                          selPass->findSink("Selected Geometry"));
    mRenderGraph->addLink(camera->internalSource(),
                          selPass->findSink("Camera"));
    mRenderGraph->addLink(selPass->findSource("Selection Map"),
                          finalColor->internalSink());
    if (!mRenderGraph->compile()) {
        return;
    }
    selectedGeometry = dyncast<GeometryResource*>(selGeo->resource());
    cameraBuffer = dyncast<BufferResource*>(camera->resource());
    mFinalColor = dyncast<TextureResource*>(finalColor->resource());
}

void EditorSceneRenderer2::begin(bloom::rg::FrameParameters const& frameParams,
                                 bloom::Camera const& camera) {
    mRenderer.begin(frameParams, camera);
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
    mRenderer.submit(scene);
    if (selectedGeometry) {
        submitSelectedGeometry(scene, *selectedGeometry, selectionManager);
    }
}

void EditorSceneRenderer2::draw(bloom::CommandQueue& commandQueue) {
    mRenderer.draw(commandQueue);
    mRenderGraph->allocateResources(mRenderer.getFrameParams());
    if (selectedGeometry) {
        selectedGeometry->prepare(mRenderGraph->device());
    }
    if (cameraBuffer && cameraBuffer->handle()) {
        auto camera = mRenderer.getCamera();
        CameraInput value = {
            .camera = vml::transpose(camera.viewProjection()),
            .cameraPosition = camera.position(),
        };
        mRenderGraph->device().fillManagedBuffer(cameraBuffer->handle(), &value,
                                                 sizeof value);
    }
    mRenderGraph->execute(mRenderer.getFrameParams(), commandQueue);
}

bloom::TextureView EditorSceneRenderer2::finalColor() const {
    return mFinalColor ? mFinalColor->handle() : nullptr;
}

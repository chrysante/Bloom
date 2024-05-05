#include "Poppy/Renderer/EditorSceneRenderer2.h"

#include "Bloom/GPU/BlitContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Renderer/Passes.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
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

EditorSceneRenderer2::EditorSceneRenderer2(
    std::unique_ptr<RenderGraph> sceneRenderGraphOwner) {
    auto* sceneRenderGraph = sceneRenderGraphOwner.get();
    mRenderGraph = std::make_unique<RenderGraph>("Editor Render Graph",
                                                 sceneRenderGraph->context(),
                                                 sceneRenderGraph->device());
    wrapRenderGraph(*mRenderGraph, std::move(sceneRenderGraphOwner));
    auto* selPass = mRenderGraph->addPass(makeSelectionPass());
    auto* idPass = mRenderGraph->addPass(makeIDPass());
    auto* compPass = mRenderGraph->addPass(makeCompositionPass());

    auto* selGeo = mRenderGraph->declareSink(
        GeometryInterface{ "Selected Geometry", nullptr });
    auto* idSink =
        mRenderGraph->declareSink(BufferInterface{ "ID Buffer", nullptr, {} });
    auto* camera = mRenderGraph->findInputSource("Camera");
    auto* compositeColor = mRenderGraph->declareSource(
        TextureInterface{ "Composite Color", nullptr, {} });
    auto* idMapSource =
        mRenderGraph->declareSource(TextureInterface{ "ID Map", nullptr, {} });
    auto* sceneColor = sceneRenderGraph->findSource("Color");
    mRenderGraph->addLink(selGeo->internalSource(),
                          selPass->findSink("Selected Geometry"));
    mRenderGraph->addLink(camera, selPass->findSink("Camera"));
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
    mRenderGraph->addLink(mRenderGraph->findInputSource("Geometry"),
                          idPass->findSink("Geometry"));
    mRenderGraph->addLink(mRenderGraph->findInputSource("Camera"),
                          idPass->findSink("Camera"));
    mRenderGraph->addLink(idSink->internalSource(),
                          idPass->findSink("ID Buffer"));
    mRenderGraph->addLink(idPass->findSource("Map"),
                          idMapSource->internalSink());
    if (!mRenderGraph->compile()) {
        return;
    }
    mRenderGraph->allocateBackingResources();
    selectedGeometry = dyncast<GeometryResource*>(selGeo->resource());
    idBuffer = dyncast<BufferResource*>(idSink->resource());
    idMap = dyncast<TextureResource*>(idMapSource->resource());
    mCompositeColor = dyncast<TextureResource*>(compositeColor->resource());
    mSceneColor = dyncast<TextureResource*>(sceneColor->resource());
    mRenderer = std::make_unique<rg::SceneRenderer>(mRenderGraph.get());
}

void EditorSceneRenderer2::begin(bloom::rg::FrameParameters const& frameParams,
                                 bloom::Camera const& camera) {
    mRenderer->begin(frameParams, camera);
    if (selectedGeometry) {
        selectedGeometry->clear();
    }
    idBufContents.clear();
    sceneIndex = 0;
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

namespace {

struct EntityHandleEncoding {
    EntityHandleEncoding() { std::memset(this, 0xff, sizeof *this); }

    EntityHandleEncoding(size_t sceneIdx, EntityID ID) {
        BL_ASSERT(ID.raw() < (1 << 24));
        entityID = ID.raw();
        sceneIndex = utl::narrow_cast<uint8_t>(sceneIdx);
    }

    bool valid() const {
        EntityHandleEncoding ref;
        return std::memcmp(this, &ref, sizeof ref) != 0;
    }

    uint32_t entityID   : 24;
    uint32_t sceneIndex : 8;
};

} // namespace

static void submitEntityIDs(size_t sceneIndex, Scene const& scene,
                            std::vector<uint32_t>& dest) {
    auto view = scene.view<TransformMatrixComponent const,
                           MeshRendererComponent const>();
    view.each([&](EntityID ID, TransformMatrixComponent const&,
                  MeshRendererComponent const& meshRenderer) {
        if (!scene.getHandle(ID).isVisible() || !meshRenderer.mesh ||
            !meshRenderer.materialInstance ||
            !meshRenderer.materialInstance->material())
        {
            return;
        }
        dest.push_back(
            std::bit_cast<uint32_t>(EntityHandleEncoding{ sceneIndex, ID }));
    });
}

void EditorSceneRenderer2::submit(
    bloom::Scene const& scene, EntitySelectionManager const& selectionManager) {
    mRenderer->submit(scene);
    if (selectedGeometry) {
        submitSelectedGeometry(scene, *selectedGeometry, selectionManager);
    }
    submitEntityIDs(sceneIndex++, scene, idBufContents);
}

void EditorSceneRenderer2::draw(bloom::CommandQueue& commandQueue) {
    mRenderer->draw(commandQueue);
    mRenderGraph->allocateResources(mRenderer->getFrameParams(),
                                    mRenderGraph->device());
    if (selectedGeometry) {
        selectedGeometry->prepare(mRenderGraph->device());
    }
    if (idBuffer) {
        auto& device = mRenderGraph->device();
        size_t bufSize = idBufContents.size() * sizeof(uint32_t);
        if (idBuffer->handle().size() < bufSize) {
            idBuffer->assign(device.createBuffer(
                { .size = bufSize, .storageMode = StorageMode::Managed }));
        }
        device.fillManagedBuffer(idBuffer->handle(), idBufContents.data(),
                                 bufSize);
    }
    mRenderGraph->execute(mRenderer->getFrameParams(), commandQueue);
}

bloom::TextureView EditorSceneRenderer2::sceneColor() const {
    return mSceneColor ? mSceneColor->handle() : nullptr;
}

bloom::TextureView EditorSceneRenderer2::sceneColorWithOverlays() const {
    return mCompositeColor ? mCompositeColor->handle() : nullptr;
}

bloom::EntityHandle EditorSceneRenderer2::readEntityIDMap(
    CommandQueue& commandQueue, std::span<Scene* const> scenes,
    float2 position) const {
    if (!idMap) {
        return {};
    }
    auto idTex = idMap->handle();
    auto ctx = commandQueue.createBlitContext();
    ctx->begin();
    ctx->synchronizeTexture(idTex, 0, 0);
    ctx->end();
    ctx->commit();
    ctx->waitUntilCompleted();
    EntityHandleEncoding encodedID;
    vml::AABB<uint32_t, 3> region = { uint3(position, 0),
                                      uint3(position, 0) + uint3(1) };
    copyData(RenderAPI::Metal, idTex, &encodedID, 4, region, 0);
    if (!encodedID.valid() || encodedID.sceneIndex >= scenes.size()) {
        return {};
    }
    auto* scene = scenes[encodedID.sceneIndex];
    return scene->getHandle(EntityID(encodedID.entityID));
}

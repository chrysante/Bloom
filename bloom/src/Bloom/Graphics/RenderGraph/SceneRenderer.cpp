#include "Bloom/Graphics/RenderGraph/SceneRenderer.h"

#include <utl/functional.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Shader/PostProcessing.h"

using namespace bloom;
using namespace rg;

rg::SceneRenderer::SceneRenderer(RenderGraph* renderGraph):
    mRenderGraph(renderGraph) {
    geometry = cast<rg::GeometryResource*>(
        renderGraph->findInputSource("Geometry")->resource());
    lights = cast<rg::LightsResource*>(
        renderGraph->findInputSource("Lights")->resource());
    sceneParams = cast<rg::BufferResource*>(
        renderGraph->findInputSource("SceneParameters")->resource());
}

void rg::SceneRenderer::begin(FrameParameters const& frameParams,
                              Camera const& camera) {
    this->frameParams = frameParams;
    this->camera = camera;
    geometry->clear();
    lights->clear();
}

template <typename LightComponent>
static void submitLights(rg::LightsResource& lights, Scene const& scene,
                         auto&& lightModifier) {
    auto view =
        scene.view<TransformMatrixComponent const, LightComponent const>();
    view.each([&](auto /* ID */, TransformMatrixComponent const& transform,
                  LightComponent light) {
        lightModifier(transform, light);
        lights.add(light.light);
    });
}

template <typename Light>
static void submitLights(rg::LightsResource& lights, Scene const& scene) {
    submitLights<Light>(lights, scene, [](auto&...) {});
}

static void submitScene(Scene const& scene, rg::GeometryResource& geometry) {
    auto view = scene.view<TransformMatrixComponent const,
                           MeshRendererComponent const>();
    view.each([&](auto /* ID */, TransformMatrixComponent const& transform,
                  MeshRendererComponent const& meshRenderer) {
        if (!meshRenderer.mesh || !meshRenderer.materialInstance ||
            !meshRenderer.materialInstance->material())
        {
            return;
        }
        geometry.add(meshRenderer.mesh->getRenderer(),
                     meshRenderer.materialInstance, transform.matrix);
    });
}

void rg::SceneRenderer::submit(Scene const& scene) {
    submitScene(scene, *geometry);
    submitLights<PointLightComponent>(*lights, scene,
                                      [](auto& transform, auto& light) {
        light.light.position = transform.matrix.column(3).xyz;
    });
    submitLights<SpotLightComponent>(*lights, scene,
                                     [](auto& transform, auto& light) {
        light.light.position = transform.matrix.column(3).xyz;
        light.light.direction =
            vml::normalize((transform.matrix * vml::float4{ 1, 0, 0, 0 }).xyz);
    });
    submitLights<DirectionalLightComponent>(*lights, scene,
                                            [](auto& transform, auto& light) {
        light.light.direction =
            vml::normalize((transform.matrix * vml::float4{ 0, 0, 1, 0 }).xyz);
    });
    submitLights<SkyLightComponent>(*lights, scene);
}

namespace {
struct BloomSettings {
    bool physicallyCorrect = true;
    float intensity = 1;
    float threshold = 1;
    float knee = 0.1;
    float clamp = 100;
    float diffusion = 1;
    float contribution = 0.1;
};
} // namespace

static BloomParameters makeBloomParams(BloomSettings settings) {
    BloomParameters params{};
    params.intensity = settings.intensity;
    params.curve = { settings.threshold - settings.knee, 2 * settings.knee,
                     0.25 / settings.knee };
    params.physicallyCorrect = settings.physicallyCorrect;
    params.threshold = settings.threshold;
    params.scale = settings.diffusion;
    params.clamp = settings.clamp;
    params.contribution = settings.contribution;
    return params;
}

void rg::SceneRenderer::draw(CommandQueue& commandQueue) {
    auto& device = mRenderGraph->device();
    lights->prepare(device);
    geometry->prepare(mRenderGraph->device());
    mRenderGraph->allocateResources(frameParams);
    SceneRenderParameters sceneParams = {
        .camera = vml::transpose(camera.viewProjection()),
        .cameraPosition = camera.position(),
        .screenSize = frameParams.screenSize,
        .screenResolution = frameParams.screenSize
    };
    auto* sceneParamBuffer = cast<rg::BufferResource*>(
        mRenderGraph->findInputSource("SceneParameters")->resource());
    BL_ASSERT(sceneParamBuffer && sceneParamBuffer->handle());
    device.fillManagedBuffer(sceneParamBuffer->handle(), &sceneParams,
                             sizeof sceneParams);
    BloomParameters bloomParams = makeBloomParams({
        .physicallyCorrect = true,
        .clamp = 100,
        .diffusion = 1,
        .contribution = 0.1,
    });
    auto* bloomParamBuffer = cast<rg::BufferResource*>(
        mRenderGraph->findInputSource("BloomParameters")->resource());
    BL_ASSERT(bloomParamBuffer && bloomParamBuffer->handle());
    device.fillManagedBuffer(bloomParamBuffer->handle(), &bloomParams,
                             sizeof bloomParams);
    PostProcessParameters ppParams{ .toneMappingFunction =
                                        ToneMappingFunction::ACES };
    auto* ppParamBuffer = cast<rg::BufferResource*>(
        mRenderGraph->findInputSource("PostProcessParameters")->resource());
    BL_ASSERT(ppParamBuffer && ppParamBuffer->handle());
    device.fillManagedBuffer(ppParamBuffer->handle(), &ppParams,
                             sizeof ppParams);
    mRenderGraph->execute(frameParams, commandQueue);
}

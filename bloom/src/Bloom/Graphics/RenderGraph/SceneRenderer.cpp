#include "Bloom/Graphics/RenderGraph/SceneRenderer.h"

#include <utl/functional.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace rg;
using namespace vml::short_types;

template <typename R>
static R* findResource(RenderGraph* renderGraph, std::string_view name) {
    auto* source = renderGraph->findInputSource(name);
    if (!source) {
        Logger::Warn("Resource not found: ", name);
        return nullptr;
    }
    return dyncast<R*>(source->resource());
}

rg::SceneRenderer::SceneRenderer(RenderGraph* renderGraph):
    mRenderGraph(renderGraph) {
    geometry = findResource<rg::GeometryResource>(renderGraph, "Geometry");
    lights = findResource<rg::LightsResource>(renderGraph, "Lights");
    cameraBuffer = findResource<rg::BufferResource>(renderGraph, "Camera");
    lightSpaceTransformBuffer =
        findResource<rg::BufferResource>(renderGraph, "LightSpaceTransforms");
    shadowMetadataBuffer =
        findResource<rg::BufferResource>(renderGraph, "ShadowMetadata");
    bloomParams =
        findResource<rg::BufferResource>(renderGraph, "BloomParameters");
    postProcessParams =
        findResource<rg::BufferResource>(renderGraph, "PostProcessParameters");
}

void rg::SceneRenderer::begin(FrameParameters const& frameParams,
                              Camera const& camera) {
    this->frameParams = frameParams;
    this->camera = camera;
    if (geometry) {
        geometry->clear();
    }
    if (lights) {
        lights->clear();
    }
    lightSpaceTransforms.clear();
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

static float4x4 makeDirectionalLightSpaceTransform(Camera const& camera,
                                                   float dist, float zDist,
                                                   float3 lightDirection) {
    float4x4 const lsProj =
        vml::ortho<vml::right_handed>(-dist, dist, -dist, dist, -zDist, zDist);

    float4x4 const lsView =
        vml::look_at<vml::right_handed>(camera.position(),
                                        camera.position() - lightDirection,
                                        { 0, 1, 0 });
    return lsProj * lsView;
}

void rg::SceneRenderer::submit(Scene const& scene) {
    if (geometry) {
        submitScene(scene, *geometry);
    }
    if (lights) {
        submitLights<PointLightComponent>(*lights, scene,
                                          [](auto& transform, auto& light) {
            light.light.position = transform.matrix.column(3).xyz;
        });
        submitLights<SpotLightComponent>(*lights, scene,
                                         [](auto& transform, auto& light) {
            light.light.position = transform.matrix.column(3).xyz;
            light.light.direction = vml::normalize(
                (transform.matrix * vml::float4{ 1, 0, 0, 0 }).xyz);
        });
        shadowMetadata.numShadowCasters = 0;
        submitLights<DirectionalLightComponent>(
            *lights, scene,
            [&](TransformMatrixComponent const& transform,
                DirectionalLightComponent& comp) {
            auto& light = comp.light;
            light.direction = vml::normalize(
                (transform.matrix * vml::float4{ 0, 0, 1, 0 }).xyz);
            float distance = light.shadowDistance;
            for (int i = 0; i < light.numCascades; ++i) {
                auto lightSpaceTransform =
                    makeDirectionalLightSpaceTransform(camera, distance,
                                                       light.shadowDistanceZ,
                                                       light.direction);
                lightSpaceTransforms.push_back(
                    vml::transpose(lightSpaceTransform));
                distance *= light.cascadeDistributionExponent;
            }
            shadowMetadata.numCascades[shadowMetadata.numShadowCasters++] =
                light.numCascades;
        });
        submitLights<SkyLightComponent>(*lights, scene);
    }
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
    mRenderGraph->allocateResources(frameParams);
    auto& device = mRenderGraph->device();
    if (geometry) {
        geometry->prepare(mRenderGraph->device());
    }
    if (lights) {
        lights->prepare(device);
    }
    if (cameraBuffer && cameraBuffer->handle()) {
        CameraInput value = {
            .camera = vml::transpose(camera.viewProjection()),
            .cameraPosition = camera.position(),
        };
        device.fillManagedBuffer(cameraBuffer->handle(), &value, sizeof value);
    }
    if (shadowMetadataBuffer && shadowMetadataBuffer->handle()) {
        device.fillManagedBuffer(shadowMetadataBuffer->handle(),
                                 &shadowMetadata, sizeof shadowMetadata);
    }
    if (lightSpaceTransformBuffer) {
        size_t byteSize = std::max(size_t{ 1 }, lightSpaceTransforms.size()) *
                          sizeof(float4x4);
        if (lightSpaceTransformBuffer->handle().size() < byteSize) {
            lightSpaceTransformBuffer->assign(device.createBuffer(
                { .size = byteSize, .storageMode = StorageMode::Managed }));
        }
        device.fillManagedBuffer(lightSpaceTransformBuffer->handle(),
                                 lightSpaceTransforms.data(), byteSize);
    }
    if (bloomParams && bloomParams->handle()) {
        BloomParameters values = makeBloomParams({
            .physicallyCorrect = true,
            .clamp = 100,
            .diffusion = 1,
            .contribution = 0.1,
        });
        device.fillManagedBuffer(bloomParams->handle(), &values, sizeof values);
    }
    if (postProcessParams && postProcessParams->handle()) {
        PostProcessParameters values{ .toneMappingFunction =
                                          ToneMappingFunction::ACES };
        device.fillManagedBuffer(postProcessParams->handle(), &values,
                                 sizeof values);
    }
    mRenderGraph->execute(frameParams, commandQueue);
}

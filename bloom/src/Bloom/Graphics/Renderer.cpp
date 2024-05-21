#include "Bloom/Graphics/Renderer.h"

#include <utl/functional.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace vml::short_types;

// clang-format off

BEGIN_RENDERGRAPH_SINK_DECL(Renderer) {
    GeometryInterface{
        "Geometry",
        nullptr,
    },
    LightsInterface{
        "Lights",
        nullptr
    },
    BufferInterface{
        "Camera",
        nullptr,
        { .size = sizeof(CameraInput) }
    },
    BufferInterface{
        "Light Space Transforms",
        nullptr,
        {}
    },
    BufferInterface{
        "Shadow Metadata",
        nullptr,
        { .size = sizeof(ShadowRenderMetadata) }
    },
    BufferInterface{
        "Bloom Parameters",
        nullptr,
        { .size = sizeof(BloomParameters) }
    },
    BufferInterface{
        "Post Process Parameters",
        nullptr,
        { .size = sizeof(PostProcessParameters) }
    }
}
END_DECL()

BEGIN_RENDERGRAPH_SOURCE_DECL(Renderer) {
    TextureInterface{ "Color", nullptr, {} },
    TextureInterface{ "Depth", nullptr, {} },
    TextureInterface{ "ID Map", nullptr, {} },
}
END_DECL()

    // clang-format on

    template <typename R>
    static R* getResource(RenderGraph* renderGraph, size_t index) {
    auto* source = renderGraph->inputSource(index);
    BL_ASSERT(source);
    return cast<R*>(source->resource());
}

Renderer::Renderer(RenderGraph* renderGraph): mRenderGraph(renderGraph) {
    geometry = getResource<GeometryResource>(renderGraph, 0);
    lights = getResource<LightsResource>(renderGraph, 1);
    cameraBuffer = getResource<BufferResource>(renderGraph, 2);
    lightSpaceTransformBuffer = getResource<BufferResource>(renderGraph, 3);
    shadowMetadataBuffer = getResource<BufferResource>(renderGraph, 4);
    bloomParams = getResource<BufferResource>(renderGraph, 5);
    postProcessParams = getResource<BufferResource>(renderGraph, 6);
}

void Renderer::begin(FrameParameters const& frameParams, Camera const& camera) {
    this->frameParams = frameParams;
    this->camera = camera;
    if (geometry) {
        geometry->clear();
    }
    if (lights) {
        lights->clear();
    }
    lightSpaceTransforms.clear();
    sceneIndex = 0;
}

template <typename LightComponent>
static void submitLights(LightsResource& lights, Scene const& scene,
                         auto&& lightModifier) {
    auto view =
        scene.view<TransformMatrixComponent const, LightComponent const>();
    view.each([&](EntityID ID, TransformMatrixComponent const& transform,
                  LightComponent light) {
        if (auto* vis = scene.tryGetComponent<VisibilityComponent>(ID);
            vis && !vis->value)
        {
            return;
        }
        lightModifier(transform, light);
        lights.add(light);
    });
}

template <typename Light>
static void submitLights(LightsResource& lights, Scene const& scene) {
    submitLights<Light>(lights, scene, [](auto&...) {});
}

static void submitGeometry(size_t sceneIndex, Scene const& scene,
                           GeometryResource& geometry) {
    auto view = scene.view<TransformMatrixComponent const,
                           MeshRendererComponent const>();
    view.each([&](EntityID ID, TransformMatrixComponent const& transform,
                  MeshRendererComponent const& meshRenderer) {
        // FIXME: MeshRendererComponent should only have a StaticMeshRenderer
        // and not a whole StaticMesh
        if (!scene.getHandle(ID).isVisible() || !meshRenderer.mesh ||
            !meshRenderer.mesh->getRenderer() ||
            !meshRenderer.materialInstance ||
            !meshRenderer.materialInstance->material() ||
            !meshRenderer.materialInstance->material()->rgMainPass)
        {
            return;
        }
        geometry.add(meshRenderer.mesh->getRenderer(),
                     meshRenderer.materialInstance, transform.matrix,
                     EntityHandleEncoding(sceneIndex, ID.raw()).raw());
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

void Renderer::submit(Scene const& scene) {
    if (geometry) {
        submitGeometry(sceneIndex++, scene, *geometry);
    }
    if (lights) {
        submitLights<PointLightComponent>(*lights, scene,
                                          [](auto& transform, auto& light) {
            light.position = transform.matrix.column(3).xyz;
        });
        submitLights<SpotLightComponent>(*lights, scene,
                                         [](auto& transform, auto& light) {
            light.position = transform.matrix.column(3).xyz;
            light.direction = vml::normalize(
                (transform.matrix * vml::float4{ 1, 0, 0, 0 }).xyz);
        });
        shadowMetadata.numShadowCasters = 0;
        submitLights<DirectionalLightComponent>(
            *lights, scene,
            [&](TransformMatrixComponent const& transform,
                DirectionalLightComponent& light) {
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
            frameParams.numShadowMaps += light.numCascades;
        });
        submitLights<SkyLightComponent>(*lights, scene);
    }
}

void Renderer::setIBLTexture(Reference<Texture2D> tex) {
    if (tex && lights) {
        lights->setIrradianceCoefficients(tex->irradianceSHCoefficients);
    }
}

namespace {

struct BloomSettings {
    bool physicallyCorrect;
    float intensity;
    float threshold;
    float knee;
    float clamp;
    float diffusion;
    float contribution;
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

static CameraInput makeCameraInput(Camera const& camera) {
    auto viewProj = vml::transpose(camera.viewProjection());
    return {
        .camera = viewProj,
        .inverseCamera = vml::inverse(viewProj),
        .cameraPosition = camera.position(),
    };
}

void Renderer::draw(CommandBuffer& commandBuffer) {
    auto& device = mRenderGraph->device();
    mRenderGraph->allocateResources(frameParams, device);
    if (geometry) {
        geometry->prepare(mRenderGraph->device());
    }
    if (lights) {
        lights->prepare(device);
    }
    if (cameraBuffer && cameraBuffer->handle()) {
        auto value = makeCameraInput(camera);
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
        if (!lightSpaceTransforms.empty()) {
            device.fillManagedBuffer(lightSpaceTransformBuffer->handle(),
                                     lightSpaceTransforms.data(), byteSize);
        }
    }
    if (bloomParams && bloomParams->handle()) {
        BloomParameters values = makeBloomParams({
            .physicallyCorrect = false,
            .intensity = 0.7,
            .threshold = 1,
            .knee = 0.1,
            .clamp = 100,
            .diffusion = 1,
        });
        device.fillManagedBuffer(bloomParams->handle(), &values, sizeof values);
    }
    if (postProcessParams && postProcessParams->handle()) {
        PostProcessParameters values{ .toneMappingFunction =
                                          ToneMappingFunction::ACES };
        device.fillManagedBuffer(postProcessParams->handle(), &values,
                                 sizeof values);
    }
    mRenderGraph->execute(frameParams, commandBuffer);
}

#include "Bloom/Graphics/Renderer.h"

#include <utl/functional.hpp>

#include "Bloom/Asset/CubemapRenderer.h"
#include "Bloom/GPU/CommandBuffer.h"
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
        "Bloom Parameters",
        nullptr,
        { .size = sizeof(BloomParameters) }
    },
    BufferInterface{
        "Post Process Parameters",
        nullptr,
        { .size = sizeof(PostProcessParameters) }
    },
    TextureInterface{
        "Specular Texture",
        nullptr,
        {
            .type = TextureType::CubeTexture,
            .size = { 512, 512, 1 },
            .pixelFormat = PixelFormat::RGBA32Float,
        }
    },
    TextureInterface{
        "BDRF Texture",
        nullptr,
        {
            .size = { 512, 512, 1 },
            .pixelFormat = PixelFormat::RGBA32Float,
            .usage = TextureUsage::ShaderRead | TextureUsage::ShaderWrite
        }
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
    static R* getResource(RenderGraph& renderGraph, size_t index) {
    auto* source = renderGraph.inputSource(index);
    BL_ASSERT(source);
    return cast<R*>(source->resource());
}

Renderer::Renderer(RenderGraph* renderGraph, CommandQueue& commandQueue):
    mRenderGraph(renderGraph) {
    BL_ASSERT(renderGraph);
    geometry = getResource<GeometryResource>(*renderGraph, 0);
    lights = getResource<LightsResource>(*renderGraph, 1);
    cameraBuffer = getResource<BufferResource>(*renderGraph, 2);
    lightSpaceTransformBuffer = getResource<BufferResource>(*renderGraph, 3);
    bloomParams = getResource<BufferResource>(*renderGraph, 4);
    postProcessParams = getResource<BufferResource>(*renderGraph, 5);
    specularTexture = getResource<TextureResource>(*renderGraph, 6);
    brdfTexture = getResource<TextureResource>(*renderGraph, 7);
    /// Generate BRDF map
    auto& device = renderGraph->device();
    BRDF = device.createTexture({
        .pixelFormat = PixelFormat::RGBA32Float,
        .size = { 512, 512, 1 },
        .usage = TextureUsage::ShaderWrite | TextureUsage::ShaderRead,
    });
    auto commandBuffer = commandQueue.createCommandBuffer();
    computeBDRF(device, *commandBuffer, BRDF);
    commandBuffer->commit();
    commandBuffer->waitForCompletion();
}

void Renderer::begin(FrameParameters const& frameParams, Camera const& camera,
                     BloomSettings bloomSettings,
                     PostProcessSettings postProcessSettings) {
    this->frameParams = frameParams;
    this->camera = camera;
    this->bloomSettings = bloomSettings;
    this->postProcessSettings = postProcessSettings;
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
                         auto&& lightModifier,
                         TextureResource* specularMap = nullptr) {
    auto view =
        scene.view<TransformMatrixComponent const, LightComponent const>();
    view.each([&](EntityID ID, TransformMatrixComponent const& transform,
                  LightComponent light) {
        if (!ConstEntityHandle(ID, &scene).isVisible()) {
            return;
        }
        lightModifier(transform, light);
        if constexpr (std::same_as<LightComponent, SkyLightComponent>) {
            if (light.cubemap) {
                ranges::copy(light.cubemap->irradianceSHCoefficients,
                             light.SHCoefficients);
            }
            if (specularMap && light.cubemap) {
                specularMap->assign(light.cubemap->GPUHandle);
            }
        }
        lights.add(light);
    });
}

template <typename Light>
static void submitLights(LightsResource& lights, Scene const& scene,
                         TextureResource* specularMap = nullptr) {
    submitLights<Light>(lights, scene, [](auto&...) {}, specularMap);
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
            frameParams.numShadowMaps += light.numCascades;
        });
        submitLights<SkyLightComponent>(*lights, scene, specularTexture);
        /// We reassign this every frame for now because I don't know where else
        /// to put it
        brdfTexture->assign(BRDF);
    }
}

static CameraInput makeCameraInput(Camera const& camera) {
    auto viewProj = vml::transpose(camera.viewProjection());
    return { .camera = viewProj,
             .inverseCamera = vml::inverse(viewProj),
             .position = camera.position(),
             .front = camera.front(),
             .viewportSize = camera.viewportSize() };
}

static BloomParameters makeBloomParams(BloomSettings const& settings) {
    return {
        .intensity = settings.intensity,
        .curve = { settings.threshold - settings.knee, 2 * settings.knee,
                   0.25 / settings.knee },
        .physicallyCorrect = settings.physicallyCorrect,
        .threshold = settings.threshold,
        .scale = settings.diffusion,
        .clamp = settings.clamp,
        .contribution = settings.contribution,
    };
}

static PostProcessParameters makePostProcessParams(
    PostProcessSettings const& settings) {
    return { .toneMappingFunction = settings.toneMappingFunction };
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
        BloomParameters values = makeBloomParams(bloomSettings);
        device.fillManagedBuffer(bloomParams->handle(), &values, sizeof values);
    }
    if (postProcessParams && postProcessParams->handle()) {
        auto value = makePostProcessParams(postProcessSettings);
        device.fillManagedBuffer(postProcessParams->handle(), &value,
                                 sizeof value);
    }
    mRenderGraph->execute(frameParams, commandBuffer);
}

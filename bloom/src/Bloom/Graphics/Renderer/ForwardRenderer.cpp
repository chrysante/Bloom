#include "Bloom/Graphics/Renderer/ForwardRenderer.h"

#include <numeric>

#include <utl/utility.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/StaticMesh.h"

using namespace mtl::short_types;
using namespace bloom;

/// MARK: Framebuffer Creation
///
///
std::unique_ptr<Framebuffer> ForwardRenderer::createFramebuffer(
    mtl::int2 size) const {
    auto framebuffer = std::make_unique<ForwardRendererFramebuffer>();
    populateFramebuffer(device(), *framebuffer, size);
    return std::move(framebuffer);
}

std::unique_ptr<Framebuffer> ForwardRenderer::createDebugFramebuffer(
    mtl::int2 size) const {
    auto framebuffer = std::make_unique<ForwardRendererDebugFramebuffer>();
    populateFramebuffer(device(), *framebuffer, size);
    TextureDescription desc;
    desc.size = { size, 1 };
    desc.type = TextureType::Texture2D;
    desc.mipmapLevelCount = 1;
    desc.storageMode = StorageMode::GPUOnly;
    desc.usage = TextureUsage::ShaderRead | TextureUsage::RenderTarget;
    desc.pixelFormat = PixelFormat::RGBA8Unorm;
    framebuffer->shadowCascade = device().createTexture(desc);

    return std::move(framebuffer);
}

void ForwardRenderer::populateFramebuffer(
    HardwareDevice& device, ForwardRendererFramebuffer& framebuffer,
    mtl::usize2 size) const {
    framebuffer.size = size;
    TextureDescription desc;
    desc.size = { size, 1 };
    desc.type = TextureType::Texture2D;
    desc.mipmapLevelCount = 1;
    desc.storageMode = StorageMode::GPUOnly;
    desc.usage = TextureUsage::ShaderRead | TextureUsage::RenderTarget;

    desc.pixelFormat = PixelFormat::Depth32Float;
    framebuffer.depth = device.createTexture(desc);

    desc.pixelFormat = PixelFormat::RGBA32Float;
    framebuffer.rawColor = device.createTexture(desc);

    desc.pixelFormat = PixelFormat::RGBA8Unorm;
    desc.usage = TextureUsage::ShaderRead | TextureUsage::ShaderWrite;
    framebuffer.postProcessed = device.createTexture(desc);

    // Bloom / Veil
    bloomRenderer.populateFramebuffer(device, framebuffer.bloom, size);
}

/// MARK: Initialization
///
///
ForwardRenderer::ForwardRenderer(bloom::Receiver receiver):
    Renderer(std::move(receiver)) {
    listen([this](ReloadShadersCommand) {
        device().reloadDefaultLibrary();
        createGPUState(device());
    });
}

static RenderPipelineHandle createShadowPipeline(HardwareDevice& device) {
    RenderPipelineDescription desc;
    desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
    desc.vertexFunction = device.createFunction("shadowVertexShader");

    desc.rasterSampleCount = 1;
    desc.inputPrimitiveTopology = PrimitiveTopologyClass::Triangle;

    return device.createRenderPipeline(desc);
}

static ComputePipelineHandle createPostprocessPipeline(HardwareDevice& device) {
    // ---------------
    ComputePipelineDescription desc;

    desc.computeFunction = device.createFunction("postprocess");

    return device.createComputePipeline(desc);
}

void ForwardRenderer::init(HardwareDevice& device) {
    RendererSanitizer::init();

    mDevice = &device;

    createGPUState(device);
    bloomRenderer.init(device);
}

void ForwardRenderer::createGPUState(HardwareDevice& device) {
    /* Buffers */ {
        BufferDescription desc{};
        desc.storageMode = StorageMode::Managed;

        desc.size = sizeof(RendererParameters);
        renderObjects.parameterBuffer = device.createBuffer(desc);
    }
    {
        DepthStencilDescription desc{};
        desc.depthWrite = true;
        desc.depthCompareFunction = CompareFunction::LessEqual;

        renderObjects.depthStencil = device.createDepthStencil(desc);
    }

    renderObjects.shadows.pipeline = createShadowPipeline(device);
    renderObjects.shadows.sampler = device.createSampler(SamplerDescription{});

    renderObjects.postprocessSampler =
        device.createSampler(SamplerDescription{});
    renderObjects.postprocessPipeline = createPostprocessPipeline(device);
}

/// MARK: Scene Construction
///
///
void ForwardRenderer::beginScene(Camera const& camera) {
    RendererSanitizer::beginScene();
    scene.clear();
    scene.camera = camera;
}

static auto const objectOrder = [](auto&& a, auto&& b) {
    auto& aMatInst = *a.materialInstance;
    auto* const aMat = aMatInst.material();
    auto& bMatInst = *b.materialInstance;
    auto* const bMat = bMatInst.material();

    if (aMat == bMat) { // sort by material first
        if (a.materialInstance == b.materialInstance) { // then by instance
            return a.mesh < b.mesh;                     // then by mesh
        }
        return a.materialInstance < b.materialInstance;
    }
    return aMat < bMat;
};

void ForwardRenderer::endScene() {
    RendererSanitizer::endScene();

    std::sort(scene.objects.begin(), scene.objects.end(), objectOrder);

    /* upload object data */ do
    {
        std::size_t const size = scene.objects.size() * sizeof(float4x4);
        if (size == 0) {
            break;
        }
        if (renderObjects.transformBuffer.size() < size) {
            BufferDescription desc;
            desc.size = size;
            desc.storageMode = StorageMode::Managed;
            renderObjects.transformBuffer = device().createBuffer(desc);
        }
        device().fillManagedBuffer(renderObjects.transformBuffer,
                                   scene.objects.data().transform, size);
    } while (0);

    /* upload light space transforms */ {
        // We need to have at least one item in the buffer, otherwise the shader
        // validation will complain about missing buffer bindings
        std::size_t const size =
            std::max((std::size_t)1,
                     scene.shadows.lightSpaceTransforms.size()) *
            sizeof(float4x4);

        if (renderObjects.shadows.lightSpaceTransforms.size() < size) {
            BufferDescription desc;
            desc.size = size;
            desc.storageMode = StorageMode::Managed;
            renderObjects.shadows.lightSpaceTransforms =
                device().createBuffer(desc);
        }

        device().fillManagedBuffer(renderObjects.shadows.lightSpaceTransforms,
                                   scene.shadows.lightSpaceTransforms.data(),
                                   scene.shadows.lightSpaceTransforms.size() *
                                       sizeof(float4x4));
    }
}

void ForwardRenderer::submit(Reference<StaticMeshRenderer> mesh,
                             Reference<MaterialInstance> matInst,
                             mtl::float4x4 const& transform) {
    RendererSanitizer::submit();
    assert((bool)matInst);
    assert(matInst->material());

    if (matInst->mDirty) {
        matInst->mDirty = false;
        if (matInst->mParameterBuffer.size() < sizeof(MaterialParameters)) {
            matInst->mParameterBuffer =
                device().createBuffer({ .size = sizeof(MaterialParameters),
                                        .storageMode = StorageMode::Managed });
        }
        device().fillManagedBuffer(matInst->mParameterBuffer,
                                   &matInst->mParameters,
                                   sizeof(MaterialParameters));
    }
    scene.objects.push_back(
        { mtl::transpose(transform), std::move(matInst), std::move(mesh) });
}

void ForwardRenderer::submit(PointLight const& light) {
    RendererSanitizer::submit();
    scene.pointLights.push_back(light);
}

void ForwardRenderer::submit(SpotLight const& l) {
    RendererSanitizer::submit();
    auto light = l;
    light.innerCutoff = std::cos(light.innerCutoff);
    light.outerCutoff = std::cos(light.outerCutoff);
    scene.spotLights.push_back(light);
}

static mtl::float4x4 directionalLightSpaceTransform(
    Camera const& camera, float dist, float zDist, mtl::float3 lightDirection) {
    mtl::float4x4 const lsProj =
        mtl::ortho<mtl::right_handed>(-dist, dist, -dist, dist, -zDist, zDist);

    mtl::float4x4 const lsView =
        mtl::look_at<mtl::right_handed>(camera.position(),
                                        camera.position() - lightDirection,
                                        { 0, 1, 0 });
    return lsProj * lsView;
}

void ForwardRenderer::submit(DirectionalLight const& light) {
    RendererSanitizer::submit();

    scene.dirLights.push_back(light);

    if (!light.castsShadows) {
        return;
    }

    ++scene.shadows.numShadowCasters;
    scene.shadows.numCascades.push_back(light.numCascades);

    float distance = light.shadowDistance;
    for (int i = 0; i < light.numCascades; ++i) {
        auto const lightSpaceTransform =
            directionalLightSpaceTransform(scene.camera, distance,
                                           light.shadowDistanceZ,
                                           light.direction);
        scene.shadows.lightSpaceTransforms.push_back(
            mtl::transpose(lightSpaceTransform));
        distance *= light.cascadeDistributionExponent;
    }
}

void ForwardRenderer::submit(SkyLight const& light) {
    RendererSanitizer::submit();
    scene.skyLights.push_back(light);
}

/// MARK: Draw
void ForwardRenderer::draw(Framebuffer& fb, CommandQueue& commandQueue) {
    auto& framebuffer = utl::down_cast<ForwardRendererFramebuffer&>(fb);

    /* upload parameters */ {
        RendererParameters const params = makeParameters(fb.size);
        mDevice->fillManagedBuffer(renderObjects.parameterBuffer, &params,
                                   sizeof params);
    }

    shadowMapPass(commandQueue);
    mainPass(framebuffer, commandQueue);

    bloomRenderer.render(commandQueue, framebuffer.bloom, framebuffer.rawColor,
                         renderObjects.parameterBuffer, framebuffer.size);

    postprocessPass(framebuffer, commandQueue);
}

RendererParameters ForwardRenderer::makeParameters(usize2 framebufferSize) {
    RendererParameters result;

    result.scene.camera = mtl::transpose(scene.camera.viewProjection());
    result.scene.cameraPosition = scene.camera.position();
    result.scene.screenResolution = framebufferSize;

    // Point Lights
    if (scene.pointLights.size() > 32) {
        Logger::Warn("Can't render more than 32 Point Lights");
        scene.pointLights.resize(32);
    }
    result.scene.numPointLights =
        utl::narrow_cast<uint32_t>(scene.pointLights.size());
    std::copy(scene.pointLights.begin(), scene.pointLights.end(),
              result.scene.pointLights);

    // Spotlights
    if (scene.spotLights.size() > 32) {
        Logger::Warn("Can't render more than 32 Spot Lights");
        scene.spotLights.resize(32);
    }
    result.scene.numSpotLights =
        utl::narrow_cast<uint32_t>(scene.spotLights.size());
    std::copy(scene.spotLights.begin(), scene.spotLights.end(),
              result.scene.spotLights);

    // Directional Lights
    if (scene.dirLights.size() > 32) {
        Logger::Warn("Can't render more than 32 Directional Lights");
        scene.dirLights.resize(32);
    }
    result.scene.numDirLights =
        utl::narrow_cast<uint32_t>(scene.dirLights.size());
    std::copy(scene.dirLights.begin(), scene.dirLights.end(),
              result.scene.dirLights);

    // Skylights
    if (scene.skyLights.size() > 32) {
        Logger::Warn("Can't render more than 32 Sky Lights");
        scene.skyLights.resize(32);
    }
    result.scene.numSkyLights =
        utl::narrow_cast<uint32_t>(scene.skyLights.size());
    std::copy(scene.skyLights.begin(), scene.skyLights.end(),
              result.scene.skyLights);

    result.shadowData.numShadowCasters =
        utl::narrow_cast<int>(scene.shadows.numShadowCasters);
    assert(scene.shadows.numShadowCasters == scene.shadows.numCascades.size());
    std::copy(scene.shadows.numCascades.begin(),
              scene.shadows.numCascades.end(),
              std::begin(result.shadowData.numCascades));

    result.postprocess.tonemapping = mToneMapping;
    result.postprocess.bloom = bloomRenderer.makeShaderParameters();

    return result;
}

void ForwardRenderer::mainPass(ForwardRendererFramebuffer& framebuffer,
                               CommandQueue& commandQueue) const {
    std::unique_ptr _ctx = commandQueue.createRenderContext();
    auto& ctx = *_ctx;

    RenderPassDescription desc{};
    RenderPassColorAttachmentDescription caDesc{};
    caDesc.texture = framebuffer.rawColor;
    caDesc.clearColor = mtl::colors<>::black;
    caDesc.loadAction = LoadAction::Clear;
    desc.colorAttachments.push_back(caDesc);

    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = framebuffer.depth;

    desc.depthAttachment = dDesc;
    ctx.begin(desc);

    // Vertex buffers
    ctx.setVertexBuffer(renderObjects.parameterBuffer, 0,
                        offsetof(RendererParameters, scene));

    // Fragment buffers
    ctx.setFragmentBuffer(renderObjects.parameterBuffer, 0);
    ctx.setFragmentBuffer(renderObjects.shadows.lightSpaceTransforms, 1);

    ctx.setFragmentTexture(renderObjects.shadows.shadowMaps, 0);
    ctx.setFragmentSampler(renderObjects.shadows.sampler, 0);

    ctx.setVertexBuffer(renderObjects.transformBuffer, 2);
    Material const* currentMaterial = nullptr;
    MaterialInstance const* currentMaterialInstance = nullptr;
    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto&& object: scene.objects) {
        if (object.materialInstance->material() != currentMaterial) {
            currentMaterial = object.materialInstance->material();
            ctx.setPipeline(currentMaterial->mainPass);
            ctx.setTriangleCullMode(currentMaterial->cullMode);
            ctx.setDepthStencil(renderObjects.depthStencil);
        }

        if (object.materialInstance.get() != currentMaterialInstance) {
            ctx.setFragmentBuffer(object.materialInstance->parameterBuffer(),
                                  2);
            currentMaterialInstance = object.materialInstance.get();
        }

        if (object.mesh.get() != currentMesh) {
            ctx.setVertexBuffer(object.mesh->vertexBuffer(), 1);
            currentMesh = object.mesh.get();
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

static TextureHandle createShadowMaps(HardwareDevice& device,
                                      size_t totalShadowMaps,
                                      ulong2 resolution) {
    TextureDescription desc;
    desc.type = TextureType::Texture2DArray;
    desc.size = usize3(resolution, 1);
    desc.arrayLength = totalShadowMaps;
    desc.pixelFormat = PixelFormat::Depth32Float;
    desc.usage = TextureUsage::RenderTarget | TextureUsage::ShaderRead;
    desc.storageMode = StorageMode::GPUOnly;

    return device.createTexture(desc);
}

void ForwardRenderer::shadowMapPass(CommandQueue& commandQueue) {
    if (scene.shadows.numShadowCasters == 0) {
        return;
    }
    std::size_t const numShadowMaps =
        std::accumulate(scene.shadows.numCascades.begin(),
                        scene.shadows.numCascades.end(), 0);
    if (numShadowMaps > scene.shadows.shadowMapArrayLength ||
        scene.shadows.needsNewShadowMaps)
    {
        renderObjects.shadows.shadowMaps =
            createShadowMaps(device(), numShadowMaps,
                             scene.shadows.shadowMapResolution);
        scene.shadows.shadowMapArrayLength = numShadowMaps;
        scene.shadows.needsNewShadowMaps = false;
    }
    std::unique_ptr _ctx = commandQueue.createRenderContext();
    auto& ctx = *_ctx;
    RenderPassDescription desc{};
    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = renderObjects.shadows.shadowMaps;
    desc.depthAttachment = dDesc;
    desc.renderTargetArrayLength = numShadowMaps;
    desc.renderTargetSize = scene.shadows.shadowMapResolution;

    ctx.begin(desc);

    ctx.setPipeline(renderObjects.shadows.pipeline);
    ctx.setTriangleCullMode(TriangleCullMode::Front); /// TODO: temporary
    ctx.setDepthStencil(renderObjects.depthStencil);
    ctx.setVertexBuffer(renderObjects.parameterBuffer, 0,
                        offsetof(RendererParameters, scene));
    ctx.setVertexBuffer(renderObjects.transformBuffer, 2);
    ctx.setVertexBuffer(renderObjects.shadows.lightSpaceTransforms, 3);
    StaticMeshRenderer* currentMesh = nullptr;
    for (size_t index = 0; auto&& object: scene.objects) {
        if (object.mesh.get() != currentMesh) {
            currentMesh = object.mesh.get();
            ctx.setVertexBuffer(currentMesh->vertexBuffer(), 1);
        }
        ctx.setVertexBufferOffset(2, index * sizeof(float4x4));
        BufferView indexBuffer = object.mesh->indexBuffer();
        DrawDescription desc{};
        desc.indexCount = indexBuffer.size() / 4;
        desc.indexType = IndexType::U32;
        desc.indexBuffer = indexBuffer;
        desc.instanceCount = numShadowMaps;
        ctx.draw(desc);
        ++index;
    }

    ctx.end();
    ctx.commit();
}

void ForwardRenderer::postprocessPass(ForwardRendererFramebuffer& framebuffer,
                                      CommandQueue& commandQueue) const {
    std::unique_ptr const _ctx = commandQueue.createComputeContext();
    auto& ctx = *_ctx;

    ctx.begin();

    ctx.setPipeline(renderObjects.postprocessPipeline);

    ctx.setBuffer(renderObjects.parameterBuffer, 0,
                  offsetof(RendererParameters, postprocess));
    ctx.setTexture(framebuffer.postProcessed, 0); // dest
    ctx.setTexture(framebuffer.rawColor, 1);
    ctx.setTexture(framebuffer.bloom.upsampleMips.front(), 2);

    uint2 const gridSize = framebuffer.size;

    auto const threadGroupWidth =
        renderObjects.postprocessPipeline.threadExecutionWidth;
    auto const threadGroupHeight =
        renderObjects.postprocessPipeline.maxTotalThreadsPerThreadgroup /
        threadGroupWidth;
    mtl::uint2 const threadGroupSize = {
        utl::narrow_cast<uint32_t>(threadGroupWidth),
        utl::narrow_cast<uint32_t>(threadGroupHeight)
    };

    ctx.dispatchThreads(gridSize, threadGroupSize);

    ctx.end();
    ctx.commit();
}

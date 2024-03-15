#include "Bloom/Graphics/Renderer/BloomRenderer.h"

#include <utl/utility.hpp>

#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/HardwareDevice.h"

using namespace mtl::short_types;
using namespace bloom;

void BloomRenderer::init(HardwareDevice& device) {
    ComputePipelineDescription desc;

    desc.computeFunction = device.createFunction("bloomDownsample");
    downsamplePipeline = device.createComputePipeline(desc);

    desc.computeFunction = device.createFunction("bloomUpsample");
    upsamplePipeline = device.createComputePipeline(desc);

    desc.computeFunction = device.createFunction("bloomPrefilter");
    prefilterPipeline = device.createComputePipeline(desc);
}

void BloomRenderer::populateFramebuffer(HardwareDevice& device,
                                        BloomFramebuffer& framebuffer,
                                        mtl::usize2 size) const {
    TextureDescription desc;
    desc.size = { size / 2, 1 };
    desc.type = TextureType::texture2D;
    desc.mipmapLevelCount = BloomFramebuffer::numDSMipLevels;
    desc.storageMode = StorageMode::GPUOnly;
    desc.usage = TextureUsage::shaderRead | TextureUsage::shaderWrite;
    desc.pixelFormat = PixelFormat::RG11B10Float;

    framebuffer.downsample = device.createTexture(desc);

    desc.mipmapLevelCount = BloomFramebuffer::numUSMipLevels;
    framebuffer.upsample = device.createTexture(desc);

    for (int i = 0; i < BloomFramebuffer::numDSMipLevels; ++i) {
        framebuffer.downsampleMips[i] =
            device.createSharedTextureView(framebuffer.downsample,
                                           TextureType::texture2D,
                                           PixelFormat::RG11B10Float, i, 1, 0,
                                           1);
    }
    for (int i = 0; i < BloomFramebuffer::numUSMipLevels; ++i) {
        framebuffer.upsampleMips[i] =
            device.createSharedTextureView(framebuffer.upsample,
                                           TextureType::texture2D,
                                           PixelFormat::RG11B10Float, i, 1, 0,
                                           1);
    }
}

void BloomRenderer::render(CommandQueue& commandQueue,
                           BloomFramebuffer& framebuffer, TextureView rawColor,
                           BufferView renderParameters,
                           mtl::uint2 const framebufferSize) {
    if (!settings.enabled) {
        return;
    }

    std::unique_ptr const _ctx = commandQueue.createComputeContext();
    auto& ctx = *_ctx;

    ctx.begin();

    auto threadGroupWidth = downsamplePipeline.threadExecutionWidth;
    auto threadGroupHeight =
        downsamplePipeline.maxTotalThreadsPerThreadgroup / threadGroupWidth;
    mtl::uint2 const threadGroupSize = {
        utl::narrow_cast<uint32_t>(threadGroupWidth),
        utl::narrow_cast<uint32_t>(threadGroupHeight)
    };

    constexpr int numDSMips = BloomFramebuffer::numDSMipLevels;
    constexpr int numUSMips = BloomFramebuffer::numUSMipLevels;
    std::array<uint2, numDSMips> downsampleGridSizes;
    for (uint2 fbSize = framebufferSize; auto& s: downsampleGridSizes) {
        fbSize /= 2;
        s = fbSize;
    }
    std::array<uint2, numUSMips> upsampleGridSizes;
    for (uint2 fbSize = framebufferSize; auto& s: upsampleGridSizes) {
        fbSize /= 2;
        s = fbSize;
    }

    // all bloom shaders want this
    ctx.setBuffer(renderParameters, 0,
                  offsetof(RendererParameters, postprocess.bloom));

    // prefilter pass (including first downsampling
    ctx.setPipeline(prefilterPipeline);
    ctx.setTexture(rawColor, 0);                      // source
    ctx.setTexture(framebuffer.downsampleMips[0], 1); // dest
    ctx.dispatchThreads(downsampleGridSizes[0], threadGroupSize);

    // rest of downsample passes
    ctx.setPipeline(downsamplePipeline);
    for (int i = 0; i < numDSMips - 1; ++i) {
        ctx.setTexture(framebuffer.downsampleMips[i], 0);     // source
        ctx.setTexture(framebuffer.downsampleMips[i + 1], 1); // dest
        ctx.dispatchThreads(downsampleGridSizes[i + 1], threadGroupSize);
    }

    // first upsample pass
    ctx.setPipeline(upsamplePipeline);

    ctx.setTexture(framebuffer.downsampleMips[numDSMips - 1],
                   0); // upsample source
    ctx.setTexture(framebuffer.downsampleMips[numDSMips - 2], 1); // copy source
    ctx.setTexture(framebuffer.upsampleMips[numUSMips - 1], 2);   // dest
    ctx.dispatchThreads(upsampleGridSizes[numUSMips - 1], threadGroupSize);

    // rest of upsample passes
    for (int i = numUSMips - 1; i > 0; --i) {
        ctx.setTexture(framebuffer.upsampleMips[i], 0);
        ctx.setTexture(framebuffer.downsampleMips[i - 1], 1);
        ctx.setTexture(framebuffer.upsampleMips[i - 1], 2);
        ctx.dispatchThreads(upsampleGridSizes[i - 1], threadGroupSize);
    }

    ctx.end();
    ctx.commit();
}

BloomParameters BloomRenderer::makeShaderParameters() const {
    BloomParameters params{};

    params.intensity = settings.intensity;
    params.curve = { settings.threshold - settings.knee, 2 * settings.knee,
                     0.25 / settings.knee };

    params.enabled = settings.enabled;
    params.physicallyCorrect = settings.physicallyCorrect;
    params.threshold = settings.threshold;
    params.scale = settings.diffusion;
    params.clamp = settings.clamp;
    params.contribution = settings.contribution;
    return params;
}

void BloomRenderer::setParameters(BloomRenderParameters const& params) {
    settings = params;
}

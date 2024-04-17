#include "Bloom/Graphics/RenderGraph/BloomPass.h"

#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Shader/PostProcessing.h"

using namespace bloom;
using namespace rg;
using namespace vml::short_types;

using Impl = PassImpl<BloomPass>;

static constexpr size_t NumMips = BloomPass::NumMips;

// clang-format off

BEGIN_SINK_DECL(BloomPass) {
    TextureInterface{
        "RawColor",
        &BloomPass::rawColor,
        {
            .pixelFormat = PixelFormat::RGBA32Float,
            .size = TextureSize::ScreenSize,
        }
    },
    BufferInterface{
        "Parameters",
        &BloomPass::parameters,
        { .size = sizeof(BloomParameters) }
    },
}
END_DECL()

BEGIN_SOURCE_DECL(BloomPass) {
    TextureInterface{
        "Output",
        &BloomPass::output,
        {
            .type = TextureType::Texture2D,
            .pixelFormat = PixelFormat::RGBA32Float,
            .size = TextureSize::ScreenSize
        }
    }
}
END_DECL()

// clang-format on

BloomPass::BloomPass(std::string name): ComputePass(std::move(name)) {}

namespace {

struct GridSizes {
    std::array<uint2, NumMips> downsample;
    std::array<uint2, NumMips - 1> upsample;

    static GridSizes create(uint2 frameBufferSize) {
        GridSizes result;
        for (uint2 size = frameBufferSize; auto& s: result.downsample) {
            size /= 2;
            s = size;
        }
        for (uint2 size = frameBufferSize; auto& s: result.upsample) {
            size /= 2;
            s = size;
        }
        return result;
    }
};

} // namespace

static vml::uint2 computeThreadGroupSize(
    ComputePipelineView downsamplePipeline) {
    size_t threadGroupWidth = downsamplePipeline.threadExecutionWidth;
    size_t threadGroupHeight =
        downsamplePipeline.maxTotalThreadsPerThreadgroup / threadGroupWidth;
    vml::uint2 threadGroupSize = { utl::narrow_cast<uint32_t>(threadGroupWidth),
                                   utl::narrow_cast<uint32_t>(
                                       threadGroupHeight) };
    return threadGroupSize;
}

void BloomPass::doExecute(FrameParameters const& params, ComputeContext& ctx) {
    ctx.begin();
    uint2 dsThreadGroupSize = computeThreadGroupSize(downsamplePipeline);
    auto gridSizes = GridSizes::create(params.screenSize);
    // All bloom shaders want this
    ctx.setBuffer(parameters, 0);
    // Prefilter pass (including first downsampling
    ctx.setPipeline(prefilterPipeline);
    ctx.setTexture(rawColor, 0);          // Source
    ctx.setTexture(downsampleMips[0], 1); // dest
    ctx.dispatchThreads(gridSizes.downsample[0], dsThreadGroupSize);
    // Rest of downsample passes
    ctx.setPipeline(downsamplePipeline);
    for (int i = 0; i < (int)NumMips - 2; ++i) {
        ctx.setTexture(downsampleMips[i], 0);     // Source
        ctx.setTexture(downsampleMips[i + 1], 1); // Dest
        ctx.dispatchThreads(gridSizes.downsample[i + 1], dsThreadGroupSize);
    }
    uint2 usThreadGroupSize = computeThreadGroupSize(upsamplePipeline);
    // First upsample pass
    ctx.setPipeline(upsamplePipeline);
    ctx.setTexture(downsampleMips[NumMips - 1],
                   0);                              // Upsample source
    ctx.setTexture(downsampleMips[NumMips - 2], 1); // Copy source
    ctx.setTexture(upsampleMips[NumMips - 2], 2);   // Dest
    ctx.dispatchThreads(gridSizes.upsample[NumMips - 2], usThreadGroupSize);
    // Rest of upsample passes
    for (int i = NumMips - 2; i > 0; --i) {
        ctx.setTexture(upsampleMips[i], 0);
        ctx.setTexture(downsampleMips[i - 1], 1);
        ctx.setTexture(upsampleMips[i - 1], 2);
        ctx.dispatchThreads(gridSizes.upsample[i - 1], usThreadGroupSize);
    }
    // Composite and write to dest
    ctx.setPipeline(compositePipeline);
    ctx.setBuffer(parameters, 0);
    ctx.setTexture(output, 0);
    ctx.setTexture(rawColor, 1);
    ctx.setTexture(upsampleMips.front(), 2);
    ctx.dispatchThreads(/* gridSize: */ params.screenSize,
                        computeThreadGroupSize(compositePipeline));
    ctx.end();
    ctx.commit();
}

void BloomPass::doAllocateResources(FrameParameters const& params,
                                    HardwareDevice& device) {
    /// Textures
    TextureDescription desc;
    desc.size = { params.screenSize / 2, 1 };
    desc.type = TextureType::Texture2D;
    desc.mipmapLevelCount = NumMips;
    desc.storageMode = StorageMode::GPUOnly;
    desc.usage = TextureUsage::ShaderRead | TextureUsage::ShaderWrite;
    desc.pixelFormat = PixelFormat::RG11B10Float;
    downsample = device.createTexture(desc);
    desc.mipmapLevelCount = NumMips - 1;
    upsample = device.createTexture(desc);
    /// Upsample texture is created by the render graph framework
    for (int i = 0; i < (int)NumMips; ++i) {
        downsampleMips[i] =
            device.createSharedTextureView(downsample, TextureType::Texture2D,
                                           PixelFormat::RG11B10Float, i, 1, 0,
                                           1);
    }
    for (int i = 0; i < (int)NumMips - 1; ++i) {
        upsampleMips[i] =
            device.createSharedTextureView(upsample, TextureType::Texture2D,
                                           PixelFormat::RG11B10Float, i, 1, 0,
                                           1);
    }
    /// Pipelines
    downsamplePipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgBloomDownsample") });
    upsamplePipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgBloomUpsample") });
    prefilterPipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgBloomPrefilter") });
    compositePipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgBloomComposite") });
}

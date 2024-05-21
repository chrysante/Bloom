#include <array>

#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace vml::short_types;

static constexpr size_t NumDownsampleMips = 8;
static constexpr size_t NumUpsampleMips = NumDownsampleMips - 1;

namespace {

struct BloomPass: PassImpl<BloomPass> {
    using PassImpl::PassImpl;

    explicit BloomPass(std::string name);

    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    ComputePipelineHandle prefilterPipeline;
    ComputePipelineHandle downsamplePipeline;
    ComputePipelineHandle upsamplePipeline;
    ComputePipelineHandle compositePipeline;

    // Sinks
    BufferHandle parameters;
    TextureHandle rawColor;

    // Sources
    TextureHandle output;

    TextureHandle downsample;
    std::array<TextureHandle, NumDownsampleMips> downsampleMips;
    TextureHandle upsample;
    std::array<TextureHandle, NumUpsampleMips> upsampleMips;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(BloomPass) {
    TextureInterface{
        "Raw Color",
        &BloomPass::rawColor,
        {}
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
        "Raw Color",
        &BloomPass::output,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .type = TextureType::Texture2D,
                .pixelFormat = PixelFormat::RGBA32Float,
                .size = { params.screenSize, 1 },
                .usage = TextureUsage::ShaderWrite | TextureUsage::ShaderRead
            };
        }
    }
}
END_DECL()

// clang-format on

BL_REGISTER_PASS(BloomPass, "Bloom Pass");

BloomPass::BloomPass(std::string name): PassImpl(std::move(name)) {}

namespace {

struct GridSizes {
    std::array<uint2, NumDownsampleMips> downsample;
    std::array<uint2, NumUpsampleMips> upsample;

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

void BloomPass::doExecute(FrameParameters const& params,
                          CommandBuffer& commandBuffer) {
    auto encoder = commandBuffer.makeComputeEncoder();
    uint2 dsThreadGroupSize = computeThreadGroupSize(downsamplePipeline);
    auto gridSizes = GridSizes::create(params.screenSize);
    // All bloom shaders want this
    encoder->setBuffer(parameters, 0);
    // Prefilter pass, including first downsampling
    encoder->setPipeline(prefilterPipeline);
    encoder->setTexture(rawColor, 0);          // Source
    encoder->setTexture(downsampleMips[0], 1); // Dest
    encoder->dispatchThreads(gridSizes.downsample[0], dsThreadGroupSize);
    // Rest of downsample passes
    encoder->setPipeline(downsamplePipeline);
    for (int i = 0; i < (int)NumDownsampleMips - 1; ++i) {
        encoder->setTexture(downsampleMips[i], 0);     // Source
        encoder->setTexture(downsampleMips[i + 1], 1); // Dest
        encoder->dispatchThreads(gridSizes.downsample[i + 1],
                                 dsThreadGroupSize);
    }
    uint2 usThreadGroupSize = computeThreadGroupSize(upsamplePipeline);
    // First upsample pass
    encoder->setPipeline(upsamplePipeline);
    encoder->setTexture(downsampleMips[NumDownsampleMips - 1],
                        0); // Upsample source
    encoder->setTexture(downsampleMips[NumDownsampleMips - 2],
                        1);                                    // Copy source
    encoder->setTexture(upsampleMips[NumUpsampleMips - 1], 2); // Dest
    encoder->dispatchThreads(gridSizes.upsample[NumUpsampleMips - 1],
                             usThreadGroupSize);
    // Rest of upsample passes
    for (int i = NumUpsampleMips - 1; i > 0; --i) {
        encoder->setTexture(upsampleMips[i], 0);
        encoder->setTexture(downsampleMips[i - 1], 1);
        encoder->setTexture(upsampleMips[i - 1], 2);
        encoder->dispatchThreads(gridSizes.upsample[i - 1], usThreadGroupSize);
    }
    // Composite and write to dest
    encoder->setPipeline(compositePipeline);
    encoder->setBuffer(parameters, 0);
    encoder->setTexture(output, 0);
    encoder->setTexture(rawColor, 1);
    encoder->setTexture(upsampleMips.front(), 2);
    encoder->dispatchThreads(/* gridSize: */ params.screenSize,
                             computeThreadGroupSize(compositePipeline));
    encoder->end();
}

void BloomPass::doAllocateResources(FrameParameters const& params,
                                    HardwareDevice& device) {
    /// Textures
    TextureDescription desc;
    desc.size = { params.screenSize / 2, 1 };
    desc.type = TextureType::Texture2D;
    desc.mipmapLevelCount = NumDownsampleMips;
    desc.storageMode = StorageMode::GPUOnly;
    desc.usage = TextureUsage::ShaderRead | TextureUsage::ShaderWrite;
    desc.pixelFormat = PixelFormat::RG11B10Float;
    downsample = device.createTexture(desc);
    desc.mipmapLevelCount = NumUpsampleMips;
    upsample = device.createTexture(desc);
    /// Upsample texture is created by the render graph framework
    for (int i = 0; i < (int)NumDownsampleMips; ++i) {
        downsampleMips[i] = createSharedTextureView(RenderAPI::Metal,
                                                    downsample,
                                                    TextureType::Texture2D,
                                                    PixelFormat::RG11B10Float,
                                                    /* firstMipLevel: */ i,
                                                    /* numMipLevels: */ 1,
                                                    /* firstSlice: */ 0,
                                                    /* numSlices: */ 1);
    }
    for (int i = 0; i < (int)NumUpsampleMips; ++i) {
        upsampleMips[i] = createSharedTextureView(RenderAPI::Metal, upsample,
                                                  TextureType::Texture2D,
                                                  PixelFormat::RG11B10Float,
                                                  /* firstMipLevel: */ i,
                                                  /* numMipLevels: */ 1,
                                                  /* firstSlice: */ 0,
                                                  /* numSlices: */ 1);
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

#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

struct PostProcessPass: ComputePass, PassImpl<PostProcessPass> {
    PostProcessPass(std::string name);

    void doExecute(FrameParameters const& params, ComputeContext& ctx) override;
    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    ComputePipelineHandle pipeline;
    BufferHandle parameterBuffer;
    TextureHandle rawColor;
    TextureHandle output;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(PostProcessPass) {
    TextureInterface{
        "Raw Color",
        &PostProcessPass::rawColor,
        {
            .pixelFormat = PixelFormat::RGBA32Float,
            .size = TextureSize::ScreenSize,
        }
    },
    BufferInterface{
        "Parameters",
        &PostProcessPass::parameterBuffer,
        { .size = sizeof(PostProcessParameters) }
    },
}
END_DECL()

BEGIN_SOURCE_DECL(PostProcessPass) {
    TextureInterface{
        "Output",
        &PostProcessPass::output,
        {
            .pixelFormat = PixelFormat::RGBA8Unorm,
            .size = TextureSize::ScreenSize,
        }
    }
}
END_DECL()

// clang-format on

BL_REGISTER_PASS(PostProcessPass, "Post Process Pass");

PostProcessPass::PostProcessPass(std::string name):
    ComputePass(std::move(name)) {}

void PostProcessPass::doExecute(FrameParameters const& params,
                                ComputeContext& ctx) {
    ctx.begin();
    ctx.setPipeline(pipeline);
    ctx.setBuffer(parameterBuffer, 0);
    ctx.setTexture(output, 0);
    ctx.setTexture(rawColor, 1);
    uint2 gridSize = params.screenSize;
    size_t threadGroupWidth = pipeline.threadExecutionWidth;
    size_t threadGroupHeight =
        pipeline.maxTotalThreadsPerThreadgroup / threadGroupWidth;
    vml::uint2 threadGroupSize = { utl::narrow_cast<uint32_t>(threadGroupWidth),
                                   utl::narrow_cast<uint32_t>(
                                       threadGroupHeight) };
    ctx.dispatchThreads(gridSize, threadGroupSize);
    ctx.end();
    ctx.commit();
}

void PostProcessPass::doAllocateResources(FrameParameters const&,
                                          HardwareDevice& device) {
    pipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgPostprocess") });
}

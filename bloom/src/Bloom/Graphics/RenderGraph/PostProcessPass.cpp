#include "Bloom/Graphics/RenderGraph/PostProcessPass.h"

#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Shader/PostProcessing.h"

using namespace bloom;
using namespace rg;
using namespace vml::short_types;

using Impl = PassImpl<PostProcessPass>;

template <>
Impl::InterfaceVector const Impl::Sinks = {
    BufferInterface{ "Parameters",
                     &PostProcessPass::parameterBuffer,
                     { .size = sizeof(PostProcessParameters) } },
    TextureInterface{ "RawColor",
                      &PostProcessPass::rawColor,
                      {
                          .pixelFormat = PixelFormat::RGBA32Float,
                          .size = TextureSize::ScreenSize,
                      } },
};

template <>
Impl::InterfaceVector const Impl::Sources = { TextureInterface{
    "Output",
    &PostProcessPass::output,
    {
        .pixelFormat = PixelFormat::RGBA8Unorm,
        .size = TextureSize::ScreenSize,
    } } };

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

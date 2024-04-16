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
    InterfaceWrapper{
        "Parameters", &PostProcessPass::parameterBuffer,
        BufferInterface{ .size = sizeof(PostProcessParameters) } },
    InterfaceWrapper{ "RawColor", &PostProcessPass::rawColor,
                      TextureInterface{
                          .pixelFormat = PixelFormat::RGBA32Float,
                          .size = TextureSize::ScreenSize,
                      } },
    InterfaceWrapper{ "BloomParameters", &PostProcessPass::bloomParameterBuffer,
                      BufferInterface{ .flags = InterfaceFlags::Optional,
                                       .size = sizeof(BloomParameters) } },
    InterfaceWrapper{ "BloomColor", &PostProcessPass::bloomColor,
                      TextureInterface{
                          .flags = InterfaceFlags::Optional,
                          .pixelFormat = PixelFormat::RG11B10Float,
                          .size = TextureSize::Relative2D({ 0.5, 0.5 }),
                      } },
};

template <>
Impl::InterfaceVector const Impl::Sources = { InterfaceWrapper{
    "Output", &PostProcessPass::output,
    TextureInterface{
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
    ctx.setBuffer(bloomParameterBuffer, 1);
    ctx.setTexture(output, 0);
    ctx.setTexture(rawColor, 1);
    ctx.setTexture(bloomColor, 2);
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

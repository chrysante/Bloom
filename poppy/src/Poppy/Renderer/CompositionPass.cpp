#include "Poppy/Renderer/Passes.h"

#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

struct CompositionPass: PassImpl<CompositionPass, ComputePass> {
    CompositionPass(): PassImpl("Composition") {}

    void doExecute(FrameParameters const& params, ComputeContext& ctx) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    // Inputs
    TextureHandle sceneColor;
    TextureHandle sceneDepth;
    TextureHandle selectionMap;
    TextureHandle selectionDepth;

    // Outputs
    TextureHandle finalColor;

    // Internal resources
    ComputePipelineHandle pipeline;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(CompositionPass) {
    TextureInterface{
        "Scene Color",
        &CompositionPass::sceneColor,
        {}
    },
    TextureInterface{
        "Scene Depth",
        &CompositionPass::sceneDepth,
        {}
    },
    TextureInterface {
        "Selection Map",
        &CompositionPass::selectionMap,
        {}
    },
    TextureInterface {
        "Selection Depth",
        &CompositionPass::selectionDepth,
        {}
    }
}
END_DECL()

BEGIN_SOURCE_DECL(CompositionPass) {
    TextureInterface{
        "Final Color",
        &CompositionPass::finalColor,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::RGBA8Unorm,
                .size = { params.screenSize, 1 }
            };
        }
    }
}
END_DECL()

// clang-format on

std::unique_ptr<Pass> poppy::makeCompositionPass() {
    return std::make_unique<CompositionPass>();
}

void CompositionPass::doExecute(FrameParameters const& params,
                                ComputeContext& ctx) {
    ctx.begin();
    ctx.setPipeline(pipeline);

    ctx.setTexture(finalColor, 0);
    ctx.setTexture(sceneColor, 1);
    ctx.setTexture(sceneDepth, 2);
    ctx.setTexture(selectionMap, 3);
    ctx.setTexture(selectionDepth, 4);

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

void CompositionPass::doAllocateResources(FrameParameters const&,
                                          HardwareDevice& device) {
    pipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgEditorComposition") });
}

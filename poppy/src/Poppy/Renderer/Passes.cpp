#include "Poppy/Renderer/Passes.h"

#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct SelectionPass: PassImpl<SelectionPass, RenderPass> {
    void doExecute(FrameParameters const& params, RenderContext& ctx) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    using PassImpl::PassImpl;

    SelectionPass(): PassImpl("Selection Pass") {}

    // Sinks
    GeometryResource* geometry;
    BufferHandle cameraBuffer;

    // Sources
    TextureHandle selectionMap;
    TextureHandle selectionDepth;

    // Internal
    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(SelectionPass) {
    GeometryInterface{
        "Selected Geometry",
        &SelectionPass::geometry
    },
    BufferInterface {
        "Camera",
        &SelectionPass::cameraBuffer,
        {}
    }
}
END_DECL()

BEGIN_SOURCE_DECL(SelectionPass) {
    TextureInterface{
        "Selection Map",
        &SelectionPass::selectionMap,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::R8Unorm,
                .size = { params.screenSize, 1 }
            };
        }
    },
    TextureInterface {
        "Selection Depth",
        &SelectionPass::selectionDepth,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::Depth32Float,
                .size = { params.screenSize, 1 }
            };
        }
    }
}
END_DECL()

// clang-format on

std::unique_ptr<Pass> poppy::makeSelectionPass() {
    return std::make_unique<SelectionPass>();
}

void SelectionPass::doExecute(FrameParameters const&, RenderContext& ctx) {
    RenderPassDescription desc{};
    RenderPassColorAttachmentDescription caDesc{};
    caDesc.texture = selectionMap;
    caDesc.clearColor = { 0, 0, 0, 1 };
    caDesc.loadAction = LoadAction::Clear;
    desc.colorAttachments.push_back(caDesc);
    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = selectionDepth;
    desc.depthAttachment = dDesc;
    ctx.begin(desc);

    ctx.setVertexBuffer(cameraBuffer, 0);
    ctx.setVertexBuffer(geometry->transformBuffer(), 2);
    ctx.setPipeline(pipeline);
    ctx.setTriangleCullMode(TriangleCullMode::Back);
    ctx.setDepthStencil(depthStencil);

    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        if (object.mesh.get() != currentMesh) {
            currentMesh = object.mesh.get();
            ctx.setVertexBuffer(currentMesh->vertexBuffer(), 1);
        }
        // Object transform
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

void SelectionPass::doAllocateResources(FrameParameters const&,
                                        HardwareDevice& device) {
    pipeline = device.createRenderPipeline(
        { .colorAttachments = { { .pixelFormat = PixelFormat::R8Unorm } },
          .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = device.createFunction("rgSelectionPassVS"),
          .fragmentFunction = device.createFunction("rgSelectionPassFS") });
    depthStencil = device.createDepthStencil(
        { .depthCompareFunction = CompareFunction::LessEqual,
          .depthWrite = true });
}

namespace {

struct CompositionPass: PassImpl<CompositionPass, ComputePass> {
    using PassImpl::PassImpl;

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

#if 0

namespace {

struct GridPass: PassImpl<GridPass, RenderPass> {
    void doExecute(FrameParameters const& params, RenderContext& ctx) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    using PassImpl::PassImpl;

    GridPass(): PassImpl("Grid Pass") {}

    // Sinks
    BufferHandle cameraBuffer;
    TextureHandle colorIn;
    TextureHandle depth;
    
    // Sources
    TextureHandle colorOut;

    // Internal
    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(GridPass) {
    TextureInterface {
        "Color",
        &GridPass::colorIn,
        {}
    },
    TextureInterface {
        "Depth",
        &GridPass::depth,
        {}
    }
    BufferInterface {
        "Camera",
        &GridPass::cameraBuffer,
        {}
    },
}
END_DECL()

BEGIN_SOURCE_DECL(GridPass) {
    GridPass{
        "Color",
        &GridPass::colorOut,
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

std::unique_ptr<Pass> poppy::makeGridPass() {
    return std::make_unique<GridPass>();
}

void GridPass::doExecute(FrameParameters const&, RenderContext& ctx) {
    RenderPassDescription desc{};
    desc.colorAttachments = {
        {
            .texture = colorOut,
            .loadAction = LoadAction::Load,
        }
    };
    desc.depthAttachment = { .texture = depth };
    ctx.begin(desc);

    ctx.setVertexBuffer(cameraBuffer, 0);
    ctx.setVertexBuffer(geometry->transformBuffer(), 2);
    ctx.setPipeline(pipeline);
    ctx.setTriangleCullMode(TriangleCullMode::Back);
    ctx.setDepthStencil(depthStencil);

    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        if (object.mesh.get() != currentMesh) {
            currentMesh = object.mesh.get();
            ctx.setVertexBuffer(currentMesh->vertexBuffer(), 1);
        }
        // Object transform
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

void GridPass::doAllocateResources(FrameParameters const&,
                                    HardwareDevice& device) {
    pipeline = device.createRenderPipeline(
        { .colorAttachments = { { .pixelFormat = PixelFormat::R8Unorm } },
          .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = device.createFunction("rgGridPassVS"),
          .fragmentFunction = device.createFunction("rgGridPassFS") });
    depthStencil = device.createDepthStencil(
        { .depthCompareFunction = CompareFunction::LessEqual,
          .depthWrite = false });
}

#endif

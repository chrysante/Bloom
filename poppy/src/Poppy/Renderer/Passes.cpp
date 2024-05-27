#include "Poppy/Renderer/Passes.h"

#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/Renderer2D.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct SelectionPass: PassImpl<SelectionPass> {
    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

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

void SelectionPass::doExecute(FrameParameters const&,
                              CommandBuffer& commandBuffer) {
    auto encoder = commandBuffer.makeRenderEncoder(
        { .colorAttachments = { { .texture = selectionMap,
                                  .clearColor = { 0, 0, 0, 1 },
                                  .loadAction = LoadAction::Clear } },
          .depthAttachment = { .texture = selectionDepth } });
    encoder->setVertexBuffer(cameraBuffer, 0);
    encoder->setVertexBuffer(geometry->transformBuffer(), 2);
    encoder->setPipeline(pipeline);
    encoder->setTriangleCullMode(TriangleCullMode::Back);
    encoder->setDepthStencil(depthStencil);
    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        if (object.mesh.get() != currentMesh) {
            currentMesh = object.mesh.get();
            encoder->setVertexBuffer(currentMesh->vertexBuffer(), 1);
        }
        // Object transform
        encoder->setVertexBufferOffset(2, index * sizeof(float4x4));
        BufferView indexBuffer = object.mesh->indexBuffer();
        DrawDescription desc{};
        desc.indexCount = indexBuffer.size() / 4;
        desc.indexType = IndexType::U32;
        desc.indexBuffer = indexBuffer;
        encoder->draw(desc);
        ++index;
    }
    encoder->end();
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

struct CompositionPass: PassImpl<CompositionPass> {
    using PassImpl::PassImpl;

    CompositionPass(): PassImpl("Composition") {}

    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

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
                .size = { params.screenSize, 1 },
                .usage = TextureUsage::ShaderWrite | TextureUsage::ShaderRead
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
                                CommandBuffer& commandBuffer) {
    auto encoder = commandBuffer.makeComputeEncoder();
    encoder->setPipeline(pipeline);
    encoder->setTexture(finalColor, 0);
    encoder->setTexture(sceneColor, 1);
    encoder->setTexture(sceneDepth, 2);
    encoder->setTexture(selectionMap, 3);
    encoder->setTexture(selectionDepth, 4);
    uint2 gridSize = params.screenSize;
    size_t threadGroupWidth = pipeline.threadExecutionWidth;
    size_t threadGroupHeight =
        pipeline.maxTotalThreadsPerThreadgroup / threadGroupWidth;
    vml::uint2 threadGroupSize = { utl::narrow_cast<uint32_t>(threadGroupWidth),
                                   utl::narrow_cast<uint32_t>(
                                       threadGroupHeight) };
    encoder->dispatchThreads(gridSize, threadGroupSize);
    encoder->end();
}

void CompositionPass::doAllocateResources(FrameParameters const&,
                                          HardwareDevice& device) {
    pipeline = device.createComputePipeline(
        { .computeFunction = device.createFunction("rgEditorComposition") });
}

namespace {

struct OverlayPass: PassImpl<OverlayPass> {
    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

    using PassImpl::PassImpl;

    OverlayPass(): PassImpl("Overlay Pass") {}

    Renderer2D renderer;

    // Sinks
    BufferHandle cameraBuffer;
    TextureHandle colorIn;
    TextureHandle depth;
    Geometry2DResource* geometry;

    // Sources
    TextureHandle colorOut;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(OverlayPass) {
    TextureInterface {
        "Color",
        &OverlayPass::colorIn,
        {}
    },
    TextureInterface {
        "Depth",
        &OverlayPass::depth,
        {}
    },
    BufferInterface {
        "Camera",
        &OverlayPass::cameraBuffer,
        {}
    },
    Geometry2DInterface {
        "Geometry",
        &OverlayPass::geometry,
        {}
    },
}
END_DECL()

BEGIN_SOURCE_DECL(OverlayPass) {
    TextureInterface{
        "Color",
        &OverlayPass::colorOut,
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

std::unique_ptr<Pass> poppy::makeOverlayPass() {
    return std::make_unique<OverlayPass>();
}

static void copyTexture(TextureHandle source, TextureHandle dest,
                        BlitEncoder& encoder) {
    if (source && dest && source != dest &&
        source.description().pixelFormat == dest.description().pixelFormat)
    {
        encoder.copyTexture(source, dest);
    }
    encoder.end();
}

static void addGrid(Renderer2DScene& scene) {
    float4 gridColor = { 0.5, 0.5, 0.5, 1 };
    float4 gridColorAlt = { 0.75, 0.75, 0.75, 1 };
    float lineWidth = 3;
    float lineWidthAlt = 1.5;
    for (int i = -1000, j = 0; i <= 1000; i += 100, ++j) {
        auto [xCol, yCol] = [&] {
            if (i == 0) {
                return std::pair{ float4(1, 0, 0, 1), float4(0, 1, 0, 1) };
            }
            float4 tmp = j % 2 == 0 ? gridColor : gridColorAlt;
            return std::pair{ tmp, tmp };
        }();
        float width = j % 2 == 0 ? lineWidth : lineWidthAlt;
        scene.addLine({ i, -1000, 0 }, { i, 1000, 0 }, xCol, width);
        scene.addLine({ -1000, i, 0 }, { 1000, i, 0 }, yCol, width);
    }
}

void OverlayPass::doExecute(FrameParameters const&,
                            CommandBuffer& commandBuffer) {
    auto& scene = geometry->scene;
    addGrid(scene);
    copyTexture(colorIn, colorOut, *commandBuffer.makeBlitEncoder());
    // clang-format off
    RenderPassDescription desc = {
        .colorAttachments = { 
            { .texture = colorOut, .loadAction = LoadAction::Load }
        },
        .depthAttachment = {
            .texture = depth, .loadAction = LoadAction::DontCare
        }
    }; // clang-format on
    renderer.drawInWorldCoords(scene, device(), commandBuffer, cameraBuffer,
                               desc);
}

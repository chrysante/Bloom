#include "Poppy/Renderer/Passes.h"

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

struct SelectionPass: RenderPass, PassImpl<SelectionPass> {
    void doExecute(FrameParameters const& params, RenderContext& ctx) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    SelectionPass(): RenderPass("Selection Pass") {}

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

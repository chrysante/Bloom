#include "Poppy/Renderer/Passes.h"

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

struct IDPass: RenderPass, PassImpl<IDPass> {
    IDPass(): RenderPass("ID Pass") {}

    void doExecute(FrameParameters const& params, RenderContext& ctx) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    // Sinks
    GeometryResource* geometry;
    BufferHandle cameraBuffer;
    BufferHandle idBuffer;

    // Sources
    TextureHandle idMap;
    TextureHandle depth;

    // Internal
    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(IDPass) {
    GeometryInterface{
        "Geometry",
        &IDPass::geometry
    },
    BufferInterface {
        "Camera",
        &IDPass::cameraBuffer,
        {}
    },
    BufferInterface {
        "ID Buffer",
        &IDPass::idBuffer,
        {}
    }
}
END_DECL()

BEGIN_SOURCE_DECL(IDPass) {
    TextureInterface{
        "Map",
        &IDPass::idMap,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::R32Uint,
                .size = { params.screenSize / 2, 1 }
            };
        }
    },
    TextureInterface{
        "Depth",
        &IDPass::depth,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::Depth32Float,
                .size = { params.screenSize / 2, 1 }
            };
        }
    }
}
END_DECL()

// clang-format on

std::unique_ptr<Pass> poppy::makeIDPass() { return std::make_unique<IDPass>(); }

void IDPass::doExecute(FrameParameters const&, RenderContext& ctx) {
    RenderPassDescription desc{};
    RenderPassColorAttachmentDescription caDesc{};
    caDesc.texture = idMap;
    caDesc.clearColor = { 0, 0, 0, 1 };
    caDesc.loadAction = LoadAction::Clear;
    desc.colorAttachments.push_back(caDesc);
    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = depth;
    desc.depthAttachment = dDesc;
    ctx.begin(desc);
    ctx.setVertexBuffer(cameraBuffer, 0);
    ctx.setVertexBuffer(geometry->transformBuffer(), 2);
    ctx.setFragmentBuffer(idBuffer, 0);
    ctx.setPipeline(pipeline);
    ctx.setTriangleCullMode(TriangleCullMode::Back);
    ctx.setDepthStencil(depthStencil);
    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        if (object.mesh.get() != currentMesh) {
            currentMesh = object.mesh.get();
            ctx.setVertexBuffer(currentMesh->vertexBuffer(), 1);
        }
        ctx.setVertexBufferOffset(2, index * sizeof(float4x4));
        ctx.setFragmentBufferOffset(0, index * sizeof(uint32_t));
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

void IDPass::doAllocateResources(FrameParameters const&,
                                 HardwareDevice& device) {
    pipeline = device.createRenderPipeline(
        { .colorAttachments = { { .pixelFormat = PixelFormat::R32Uint } },
          .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = device.createFunction("rgSelectionPassVS"),
          .fragmentFunction = device.createFunction("rgIDPassFS") });
    depthStencil = device.createDepthStencil(
        { .depthCompareFunction = CompareFunction::LessEqual,
          .depthWrite = true });
}

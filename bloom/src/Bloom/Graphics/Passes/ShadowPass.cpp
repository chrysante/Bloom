#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

struct ShadowPass: PassImpl<ShadowPass, RenderPass> {
    using PassImpl::PassImpl;

    ShadowPass(std::string name);

    void doExecute(FrameParameters const& params, RenderContext& ctx) override;
    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    // Sinks
    GeometryResource* geometry;
    BufferHandle lightSpaceTransforms; // Buffer of light space "cameras"

    // Sources
    TextureHandle shadowMaps; // Texture array, one element for each cascade

    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(ShadowPass) {
    GeometryInterface{
        "Geometry",
        &ShadowPass::geometry
    },
    BufferInterface{
        "Light Space Transforms",
        &ShadowPass::lightSpaceTransforms,
        {} // TODO: What to do with size? Do sinks actually need to declare size?
    }
}
END_DECL()

BEGIN_SOURCE_DECL(ShadowPass) {
    TextureInterface{
        "Shadow Maps",
        &ShadowPass::shadowMaps,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .type = TextureType::Texture2DArray,
                .pixelFormat = PixelFormat::Depth32Float,
                .size = { params.shadowMapSize, 1 },
                .arrayLength = params.numShadowMaps
            };
        }
    }
}
END_DECL()

// clang-format on

BL_REGISTER_PASS(ShadowPass, "Shadow Pass");

ShadowPass::ShadowPass(std::string name): PassImpl(std::move(name)) {}

void ShadowPass::doExecute(FrameParameters const& frameParams,
                           RenderContext& ctx) {
    size_t numCascades = frameParams.numShadowMaps;
    if (numCascades == 0) {
        return;
    }
    BL_ASSERT(numCascades == shadowMaps.description().arrayLength);
    RenderPassDescription desc{};
    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = shadowMaps;
    desc.depthAttachment = dDesc;
    desc.renderTargetArrayLength = numCascades;
    desc.renderTargetSize = shadowMaps.size().xy;
    ctx.begin(desc);
    ctx.setPipeline(pipeline);
    ctx.setTriangleCullMode(TriangleCullMode::Front); /// TODO: temporary
    ctx.setDepthStencil(depthStencil);
    ctx.setVertexBuffer(lightSpaceTransforms, 1);
    ctx.setVertexBuffer(geometry->transformBuffer(), 2);
    StaticMeshRenderer* currentMesh = nullptr;
    for (size_t index = 0; auto&& object: geometry->getObjects()) {
        if (object.mesh.get() != currentMesh) {
            currentMesh = object.mesh.get();
            ctx.setVertexBuffer(currentMesh->vertexBuffer(), 0);
        }
        ctx.setVertexBufferOffset(2, index * sizeof(float4x4));
        BufferView indexBuffer = object.mesh->indexBuffer();
        DrawDescription desc{};
        desc.indexCount = indexBuffer.size() / 4;
        desc.indexType = IndexType::U32;
        desc.indexBuffer = indexBuffer;
        desc.instanceCount = numCascades;
        ctx.draw(desc);
        ++index;
    }
    ctx.end();
    ctx.commit();
}

void ShadowPass::doAllocateResources(FrameParameters const&,
                                     HardwareDevice& device) {
    pipeline = device.createRenderPipeline(
        { .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
          .vertexFunction = device.createFunction("rgShadowVertexShader"),
          .rasterSampleCount = 1,
          .inputPrimitiveTopology = PrimitiveTopologyClass::Triangle });
    depthStencil = device.createDepthStencil({
        .depthWrite = true,
        .depthCompareFunction = CompareFunction::LessEqual,
    });
}

#include "Bloom/Graphics/RenderGraph/ShadowPass.h"

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace rg;
using namespace vml::short_types;

static ulong2 const Resolution = { 512, 512 };

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
    },
}
END_DECL()

BEGIN_SOURCE_DECL(ShadowPass) {
    TextureInterface{
        "ShadowMaps",
        &ShadowPass::shadowMaps,
        {
            .type = TextureType::Texture2DArray,
            .pixelFormat = PixelFormat::Depth32Float,
            .size = TextureSize::Absolute2D(Resolution),
            // FIXME: Add dynamic interface descriptors
            // Setting the array length here is a hack to avoid reallocation of
            // the texture array, because the users of the texture (forward
            // shading pass) would not be notified
            .arrayLength = 20
        }
    },
}
END_DECL()

    // clang-format on

    ShadowPass::ShadowPass(std::string name):
    RenderPass(std::move(name)) {}

static TextureHandle createShadowMaps(HardwareDevice& device,
                                      size_t totalShadowMaps,
                                      ulong2 resolution) {
    TextureDescription desc;
    desc.type = TextureType::Texture2DArray;
    desc.size = usize3(resolution, 1);
    desc.arrayLength = totalShadowMaps;
    desc.pixelFormat = PixelFormat::Depth32Float;
    desc.usage = TextureUsage::RenderTarget | TextureUsage::ShaderRead;
    desc.storageMode = StorageMode::GPUOnly;
    return device.createTexture(desc);
}

void ShadowPass::doExecute(FrameParameters const&, RenderContext& ctx) {
    size_t numCascades = lightSpaceTransforms.size() / sizeof(float4x4);
    if (numCascades == 0) {
        return;
    }
    if (numCascades > shadowMaps.description().arrayLength) {
        shadowMaps = createShadowMaps(device(), numCascades, Resolution);
    }
    RenderPassDescription desc{};
    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = shadowMaps;
    desc.depthAttachment = dDesc;
    desc.renderTargetArrayLength = numCascades;
    desc.renderTargetSize = Resolution;
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

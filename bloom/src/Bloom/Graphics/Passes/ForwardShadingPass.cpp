#include <vml/vml.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace rg;
using namespace vml;

namespace {

struct ForwardShadingPass: PassImpl<ForwardShadingPass, RenderPass> {
    using PassImpl::PassImpl;

    explicit ForwardShadingPass(std::string name);

    void doExecute(FrameParameters const& params, RenderContext& ctx) override;
    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    // Sinks
    GeometryResource* geometry;
    LightsResource* lights;
    BufferHandle cameraBuffer;
    BufferHandle lightSpaceTransforms;
    BufferHandle shadowMetadata;
    TextureHandle shadowMaps;

    // Sources
    TextureHandle rawColor;
    TextureHandle depth;
    TextureHandle idMap;

    // Internal resources
    SamplerHandle shadowMapSampler;
};

} // namespace

// clang-format off

BEGIN_SINK_DECL(ForwardShadingPass) {
    GeometryInterface{
        "Geometry",
        &ForwardShadingPass::geometry
    },
    LightsInterface{
        "Lights",
        &ForwardShadingPass::lights
    },
    BufferInterface{
        "Camera",
        &ForwardShadingPass::cameraBuffer,
        { .size = sizeof(CameraInput) }
    },
    BufferInterface{
        "Light Space Transforms",
        &ForwardShadingPass::lightSpaceTransforms,
        {}
    },
    BufferInterface{
        "Shadow Metadata",
        &ForwardShadingPass::shadowMetadata,
        {}
    },
    TextureInterface{
        "Shadow Maps",
        &ForwardShadingPass::shadowMaps,
        {}
    },
}
END_DECL()

BEGIN_SOURCE_DECL(ForwardShadingPass) {
    TextureInterface{
        "Raw Color",
        &ForwardShadingPass::rawColor,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::RGBA32Float,
                .size = { params.screenSize, 1 }
            };
        }
    },
    TextureInterface{
        "Depth",
        &ForwardShadingPass::depth,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::Depth32Float,
                .size = { params.screenSize, 1 }
            };
        }
    },
    TextureInterface{
        "EntityID Map",
        &ForwardShadingPass::idMap,
        [](FrameParameters const& params) {
            return TextureInterfaceDescriptor{
                .pixelFormat = PixelFormat::R32Uint,
                .size = { params.screenSize, 1 },
                .storageMode = StorageMode::Managed
            };
        }
    }
}
END_DECL()

// clang-format on

BL_REGISTER_PASS(ForwardShadingPass, "Forward Shading Pass");

ForwardShadingPass::ForwardShadingPass(std::string name):
    PassImpl(std::move(name)) {}

void ForwardShadingPass::doExecute(FrameParameters const& params,
                                   RenderContext& ctx) {
    RenderPassDescription desc{};
    desc.colorAttachments = {
        {
            .texture = rawColor,
            .clearColor = vml::colors<>::black,
            .loadAction = LoadAction::Clear,
        },
        {
            .texture = idMap,
            .clearColor = { UINT_MAX, 0, 0, 1 },
            .loadAction = LoadAction::Clear,
        },
    };
    desc.depthAttachment = { .texture = depth };
    ctx.begin(desc);
    ctx.setVertexBuffer(cameraBuffer, 0);
    ctx.setVertexBuffer(geometry->transformBuffer(), 2);
    ctx.setFragmentBuffer(cameraBuffer, 0);
    ctx.setFragmentBuffer(lights->lightBuffer(), 1);
    ctx.setFragmentBuffer(lightSpaceTransforms, 2);
    ctx.setFragmentBuffer(shadowMetadata, 3);
    ctx.setFragmentBuffer(geometry->IDBuffer(), 5);
    ctx.setFragmentTexture(shadowMaps, 0);
    ctx.setFragmentSampler(shadowMapSampler, 0);
    if (params.drawWireframe) {
        ctx.setTriangleFillMode(TriangleFillMode::Lines);
    }
    Material const* currentMaterial = nullptr;
    MaterialInstance const* currentMaterialInstance = nullptr;
    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        object.materialInstance->uploadData(device());
        if (object.materialInstance->material() != currentMaterial) {
            currentMaterial = object.materialInstance->material();
            ctx.setPipeline(currentMaterial->rgMainPass);
            ctx.setTriangleCullMode(currentMaterial->cullMode);
            ctx.setDepthStencil(currentMaterial->depthStencil);
        }
        if (object.materialInstance.get() != currentMaterialInstance) {
            ctx.setFragmentBuffer(object.materialInstance->parameterBuffer(),
                                  4);
            currentMaterialInstance = object.materialInstance.get();
        }
        if (object.mesh.get() != currentMesh) {
            ctx.setVertexBuffer(object.mesh->vertexBuffer(), 1);
            currentMesh = object.mesh.get();
        }
        // Object transform
        ctx.setVertexBufferOffset(2, index * sizeof(float4x4));
        // Entity ID
        ctx.setFragmentBufferOffset(5, index * sizeof(uint32_t));
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

void ForwardShadingPass::doAllocateResources(FrameParameters const&,
                                             HardwareDevice& device) {

    shadowMapSampler = device.createSampler({});
}

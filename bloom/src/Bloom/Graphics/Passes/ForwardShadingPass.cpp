#include <vml/vml.hpp>

#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace vml;

namespace {

struct ForwardShadingPass: PassImpl<ForwardShadingPass> {
    using PassImpl::PassImpl;

    explicit ForwardShadingPass(std::string name);

    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

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

static int getBindIndex(MaterialShaderType shaderType,
                        GlobalMaterialBindingType bindingType) {
    return getGlobalMaterialBindingPoint(shaderType, bindingType).index;
}

void ForwardShadingPass::doExecute(FrameParameters const& params,
                                   CommandBuffer& commandBuffer) {
    auto encoder = commandBuffer.makeRenderEncoder({
        .colorAttachments = {
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
        },
        .depthAttachment = { .texture = depth }
    });
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    size_t transformBufferIndex = getBindIndex(Vertex, Transform);
    encoder->setVertexBuffer(cameraBuffer, getBindIndex(Vertex, Camera));
    encoder->setVertexBuffer(geometry->transformBuffer(), transformBufferIndex);
    encoder->setFragmentBuffer(cameraBuffer, getBindIndex(Fragment, Camera));
    encoder->setFragmentBuffer(lights->lightBuffer(),
                               getBindIndex(Fragment, Lights));
    encoder->setFragmentBuffer(lightSpaceTransforms,
                               getBindIndex(Fragment, LightSpaceTransforms));
    encoder->setFragmentBuffer(shadowMetadata,
                               getBindIndex(Fragment, ShadowMetadata));
    encoder->setFragmentBuffer(geometry->IDBuffer(),
                               getBindIndex(Fragment, GeometryIDBuffer));
    encoder->setFragmentTexture(shadowMaps, getBindIndex(Fragment, ShadowMaps));
    if (params.drawWireframe) {
        encoder->setTriangleFillMode(TriangleFillMode::Lines);
    }
    Material const* currentMaterial = nullptr;
    MaterialInstance const* currentMaterialInstance = nullptr;
    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        object.materialInstance->uploadData(device());
        if (object.materialInstance->material() != currentMaterial) {
            currentMaterial = object.materialInstance->material();
            encoder->setPipeline(currentMaterial->rgMainPass);
            encoder->setTriangleCullMode(currentMaterial->cullMode);
            encoder->setDepthStencil(currentMaterial->depthStencil);
            for (auto& [index, texture]:
                 currentMaterial->constantFragmentTextures)
            {
                if (texture) {
                    encoder->setFragmentTexture(texture->GPUHandle, index);
                }
            }
        }
        if (object.materialInstance.get() != currentMaterialInstance) {
            auto paramBuffer = object.materialInstance->parameterBuffer();
            encoder->setFragmentBuffer(paramBuffer,
                                       getBindIndex(Fragment,
                                                    MaterialParameterBuffer));
            currentMaterialInstance = object.materialInstance.get();
        }
        if (object.mesh.get() != currentMesh) {
            encoder->setVertexBuffer(object.mesh->vertexBuffer(),
                                     getBindIndex(Vertex, Geometry));
            currentMesh = object.mesh.get();
        }
        // Object transform
        encoder->setVertexBufferOffset(transformBufferIndex,
                                       index * sizeof(float4x4));
        // Entity ID
        encoder->setFragmentBufferOffset(getBindIndex(Fragment,
                                                      GeometryIDBuffer),
                                         index * sizeof(uint32_t));
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

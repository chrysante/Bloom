#include <range/v3/view.hpp>
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
using namespace ranges::views;

namespace {

struct ForwardShadingPass: PassImpl<ForwardShadingPass> {
    using PassImpl::PassImpl;

    explicit ForwardShadingPass(std::string name);

    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

    // Sinks
    GeometryResource* geometry;
    LightsResource* lights;
    BufferHandle sceneDataBuffer;
    BufferHandle lightSpaceTransforms;
    TextureHandle shadowMaps;
    TextureHandle specularTexture;
    TextureHandle brdfTexture;

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
        "Scene Data",
        &ForwardShadingPass::sceneDataBuffer,
        {}
    },
    BufferInterface{
        "Light Space Transforms",
        &ForwardShadingPass::lightSpaceTransforms,
        {}
    },
    TextureInterface{
        "Shadow Maps",
        &ForwardShadingPass::shadowMaps,
        {}
    },
    TextureInterface{
        "Specular Texture",
        &ForwardShadingPass::specularTexture,
        {}
    },
    TextureInterface{
        "BRDF Texture",
        &ForwardShadingPass::brdfTexture,
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

static void setTextureParameter(RenderEncoder& encoder,
                                MaterialTextureParameter const& param,
                                Texture2D* tex) {
    if (param.hasBinding(MaterialShaderType::ShadingFragment)) {
        auto texView = tex ? tex->GPUHandle : TextureView();
        encoder.setFragmentTexture(
            texView, param.bindingPoint[MaterialShaderType::ShadingFragment]);
    }
    if (param.hasBinding(MaterialShaderType::ShadingVertex)) {
        auto texView = tex ? tex->GPUHandle : TextureView();
        encoder.setVertexTexture(
            texView, param.bindingPoint[MaterialShaderType::ShadingVertex]);
    }
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
    size_t transformBufferIndex = getBindIndex(ShadingVertex, Transform);
    encoder->setVertexBuffer(sceneDataBuffer,
                             getBindIndex(ShadingVertex, SceneData));
    encoder->setVertexBuffer(geometry->transformBuffer(), transformBufferIndex);
    encoder->setFragmentBuffer(sceneDataBuffer,
                               getBindIndex(ShadingFragment, SceneData));
    encoder->setFragmentBuffer(lights->lightBuffer(),
                               getBindIndex(ShadingFragment, Lights));
    encoder->setFragmentBuffer(lightSpaceTransforms,
                               getBindIndex(ShadingFragment,
                                            LightSpaceTransforms));
    encoder->setFragmentBuffer(geometry->IDBuffer(),
                               getBindIndex(ShadingFragment, EntityIDBuffer));
    encoder->setFragmentTexture(shadowMaps,
                                getBindIndex(ShadingFragment, ShadowMaps));
    encoder->setFragmentTexture(specularTexture,
                                getBindIndex(ShadingFragment, SpecularTexture));
    encoder->setFragmentTexture(brdfTexture,
                                getBindIndex(ShadingFragment, BRDFTexture));
    if (params.drawWireframe) {
        encoder->setTriangleFillMode(TriangleFillMode::Lines);
    }
    Material* curMat = nullptr;
    MaterialInstance* curMatInst = nullptr;
    StaticMeshRenderer* curMesh = nullptr;
    for (size_t index = 0; auto object: geometry->getObjects()) {
        if (object.materialInstance->material() != curMat) {
            curMat = object.materialInstance->material();
            encoder->setPipeline(curMat->rgMainPass);
            encoder->setTriangleCullMode(curMat->cullMode);
            encoder->setDepthStencil(curMat->depthStencil);
            for (auto [index, param]:
                 curMat->constantTextureParameters | enumerate)
            {
                setTextureParameter(*encoder, param,
                                    curMat->constantTextures[index].get());
            }
        }
        if (object.materialInstance.get() != curMatInst) {
            curMatInst = object.materialInstance.get();
            curMatInst->uploadData(device());
            auto paramBuffer = curMatInst->buffer();
            encoder->setFragmentBuffer(paramBuffer,
                                       getBindIndex(ShadingFragment,
                                                    MaterialParameterBuffer));
            encoder->setVertexBuffer(paramBuffer,
                                     getBindIndex(ShadingVertex,
                                                  MaterialParameterBuffer));
            for (auto [index, param]:
                 curMatInst->getTextureParameters() | enumerate)
            {
                setTextureParameter(*encoder, param,
                                    curMatInst->getTexture(index).get());
            }
        }
        if (object.mesh.get() != curMesh) {
            encoder->setVertexBuffer(object.mesh->vertexBuffer(),
                                     getBindIndex(ShadingVertex, Geometry));
            curMesh = object.mesh.get();
        }
        // Object transform
        encoder->setVertexBufferOffset(transformBufferIndex,
                                       index * sizeof(float4x4));
        // Entity ID
        encoder->setFragmentBufferOffset(getBindIndex(ShadingFragment,
                                                      EntityIDBuffer),
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

#include <range/v3/view.hpp>
#include <vml/vml.hpp>

#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/Fwd.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/Passes/ShadingPassUtils.h"
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

struct ForwardShadingPass: PassImpl<ForwardShadingPass>, ShadingPassHelper {
    using PassImpl::PassImpl;

    explicit ForwardShadingPass(std::string name);

    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

    void bindMaterial(Material& mat);
    void bindMaterialInstance(MaterialInstance& curMatInst);
    void bindMesh(StaticMeshRenderer& mesh);

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

    //
    Material* curMat = nullptr;
    MaterialInstance* curMatInst = nullptr;
    StaticMeshRenderer* curMesh = nullptr;
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
    setEncoder(*encoder);
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    size_t transformBufferIndex =
        getGlobalMaterialBindingPoint(ShadingVertex, Transform).index;
    encoder->setVertexBuffer(geometry->transformBuffer(), transformBufferIndex);
    bindBuffer(sceneDataBuffer, SceneData, ShadingVertex);
    bindBuffer(sceneDataBuffer, SceneData, ShadingFragment);
    bindBuffer(lights->lightBuffer(), Lights, ShadingFragment);
    bindBuffer(lightSpaceTransforms, LightSpaceTransforms, ShadingFragment);
    bindBuffer(geometry->IDBuffer(), EntityIDBuffer, ShadingFragment);
    bindTexture(shadowMaps, ShadowMaps, ShadingFragment);
    bindTexture(specularTexture, SpecularTexture, ShadingFragment);
    bindTexture(brdfTexture, BRDFTexture, ShadingFragment);
    if (params.drawWireframe) {
        encoder->setTriangleFillMode(TriangleFillMode::Lines);
    }
    for (size_t index = 0; auto object: geometry->getObjects()) {
        bindMaterial(*object.materialInstance->material());
        bindMaterialInstance(*object.materialInstance);
        bindMesh(*object.mesh);
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
    curMat = nullptr;
    curMatInst = nullptr;
    curMesh = nullptr;
}

void ForwardShadingPass::bindMaterial(Material& mat) {
    if (curMat == &mat) {
        return;
    }
    curMat = &mat;
    encoder().setPipeline(mat.rgMainPass);
    encoder().setTriangleCullMode(mat.cullMode);
    encoder().setDepthStencil(mat.depthStencil);
    for (auto [index, param]: mat.constantTextureParameters | enumerate) {
        using enum GlobalMaterialBindingType;
        using enum MaterialShaderType;
        setTextureParameter(param, mat.constantTextures[index].get(),
                            ShadingVertex, ShadingFragment);
    }
}

void ForwardShadingPass::bindMaterialInstance(MaterialInstance& inst) {
    if (curMatInst == &inst) {
        return;
    }
    curMatInst = &inst;
    inst.uploadData(device());
    auto paramBuffer = inst.buffer();
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    bindBuffer(paramBuffer, MaterialParameterBuffer, ShadingVertex);
    bindBuffer(paramBuffer, MaterialParameterBuffer, ShadingFragment);
    for (auto [index, param]: inst.getTextureParameters() | enumerate) {
        setTextureParameter(param, inst.getTexture(index).get(), ShadingVertex,
                            ShadingFragment);
    }
}

void ForwardShadingPass::bindMesh(StaticMeshRenderer& mesh) {
    if (&mesh == curMesh) {
        return;
    }
    curMesh = &mesh;
    bindBuffer(curMesh->vertexBuffer(), GlobalMaterialBindingType::Geometry,
               MaterialShaderType::ShadingVertex);
}

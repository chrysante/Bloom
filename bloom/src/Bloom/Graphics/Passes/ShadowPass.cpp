#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/Passes/ShadingPassUtils.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

struct ShadowPass: PassImpl<ShadowPass>, ShadingPassHelper {
    using PassImpl::PassImpl;

    ShadowPass(std::string name);

    void doExecute(FrameParameters const& params,
                   CommandBuffer& commandBuffer) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    void bindMaterial(Material& mat);
    void bindMaterialInstance(MaterialInstance& curMatInst);
    void bindMesh(StaticMeshRenderer& mesh);

    // Sinks
    GeometryResource* geometry;
    BufferHandle lightSpaceTransforms;
    BufferHandle sceneDataBuffer;

    // Sources
    TextureHandle shadowMaps; // Texture array, one element for each cascade

    //
    Material* curMat = nullptr;
    MaterialInstance* curMatInst = nullptr;
    StaticMeshRenderer* curMesh = nullptr;
    TriangleCullMode currentCullMode = TriangleCullMode::Front;

    RenderPipelineHandle defaultPipeline;
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
        {}
    },
    BufferInterface{
        "Scene Data",
        &ShadowPass::sceneDataBuffer,
        {}
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

static TriangleCullMode transformCullMode(TriangleCullMode in) {
    using enum TriangleCullMode;
    switch (in) {
    case None:
        return None;
    case Back:
        return Front;
    case Front:
        return Back;
    }
}

void ShadowPass::doExecute(FrameParameters const& frameParams,
                           CommandBuffer& commandBuffer) {
    size_t numCascades = frameParams.numShadowMaps;
    if (numCascades == 0) {
        return;
    }
    BL_ASSERT(numCascades == shadowMaps.description().arrayLength);
    auto encoder = commandBuffer.makeRenderEncoder(
        { .depthAttachment = { .texture = shadowMaps },
          .renderTargetArrayLength = numCascades,
          .renderTargetSize = shadowMaps.size().xy });
    setEncoder(*encoder);
    encoder->setPipeline(defaultPipeline);
    currentCullMode = TriangleCullMode::Front;
    encoder->setTriangleCullMode(currentCullMode);
    encoder->setDepthStencil(depthStencil);
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    size_t transformBufferIndex =
        getGlobalMaterialBindingPoint(ShadowVertex, Transform).index;
    encoder->setVertexBuffer(geometry->transformBuffer(), transformBufferIndex);
    bindBuffer(lightSpaceTransforms, LightSpaceTransforms, ShadowVertex);
    bindBuffer(sceneDataBuffer, SceneData, ShadowVertex);
    bindBuffer(sceneDataBuffer, SceneData, ShadowFragment);
    for (size_t index = 0; auto&& object: geometry->getObjects()) {
        bindMaterial(*object.materialInstance->material());
        bindMaterialInstance(*object.materialInstance);
        bindMesh(*object.mesh);
        encoder->setVertexBufferOffset(transformBufferIndex,
                                       index * sizeof(float4x4));
        BufferView indexBuffer = object.mesh->indexBuffer();
        DrawDescription desc{};
        desc.indexCount = indexBuffer.size() / 4;
        desc.indexType = IndexType::U32;
        desc.indexBuffer = indexBuffer;
        desc.instanceCount = numCascades;
        encoder->draw(desc);
        ++index;
    }
    encoder->end();
}

void ShadowPass::bindMaterial(Material& mat) {
    auto cullMode = transformCullMode(mat.cullMode);
    if (cullMode != currentCullMode) {
        encoder().setTriangleCullMode(cullMode);
        currentCullMode = cullMode;
    }
    auto* ref = mat.shadowPipeline ? &mat : nullptr;
    if (curMat == ref) {
        return;
    }
    curMat = ref;
    if (ref) {
        encoder().setPipeline(mat.shadowPipeline);
        for (auto [index, param]: mat.constantTextureParameters | enumerate) {
            using enum GlobalMaterialBindingType;
            using enum MaterialShaderType;
            setTextureParameter(param, mat.constantTextures[index].get(),
                                ShadowVertex, ShadowFragment);
        }
    }
    else {
        encoder().setPipeline(defaultPipeline);
    }
}

void ShadowPass::bindMaterialInstance(MaterialInstance& inst) {
    auto* ref = inst.material()->shadowPipeline ? &inst : nullptr;
    if (curMatInst == ref) {
        return;
    }
    curMatInst = ref;
    // FIXME: We are uploading the data twice, once here and once in the shading
    // pass
    inst.uploadData(device());
    auto paramBuffer = inst.buffer();
    using enum GlobalMaterialBindingType;
    using enum MaterialShaderType;
    bindBuffer(paramBuffer, MaterialParameterBuffer, ShadowVertex);
    bindBuffer(paramBuffer, MaterialParameterBuffer, ShadowFragment);
    for (auto [index, param]: inst.getTextureParameters() | enumerate) {
        setTextureParameter(param, inst.getTexture(index).get(), ShadingVertex,
                            ShadowFragment);
    }
}

void ShadowPass::bindMesh(StaticMeshRenderer& mesh) {
    if (&mesh == curMesh) {
        return;
    }
    curMesh = &mesh;
    bindBuffer(curMesh->vertexBuffer(), GlobalMaterialBindingType::Geometry,
               MaterialShaderType::ShadowVertex);
}

void ShadowPass::doAllocateResources(FrameParameters const&,
                                     HardwareDevice& device) {
    // clang-format off
    defaultPipeline = device.createRenderPipeline({
        .depthAttachmentPixelFormat = PixelFormat::Depth32Float,
        .vertexFunction = device.createFunction("rgShadowVertexShader"),
        .inputPrimitiveTopology = PrimitiveTopologyClass::Triangle
    });
    depthStencil = device.createDepthStencil({
        .depthWrite = true,
        .depthCompareFunction = CompareFunction::LessEqual,
    });
    // clang-format on
}

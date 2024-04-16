#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"

#include <vml/vml.hpp>

#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/StaticMesh.h"

using namespace bloom;
using namespace rg;
using namespace vml;

using Impl = PassImpl<ForwardShadingPass>;

template <>
Impl::InterfaceVector const Impl::Sinks = {
    InterfaceWrapper{
        "SceneParameters", &ForwardShadingPass::sceneParameters,
        BufferInterface{ .size = sizeof(SceneRenderParameters) } },
    InterfaceWrapper{ "Geometry", &ForwardShadingPass::geometry,
                      GeometryInterface{} },
    InterfaceWrapper{ "Lights", &ForwardShadingPass::lights,
                      LightsInterface{} },
};

template <>
Impl::InterfaceVector const Impl::Sources = {
    InterfaceWrapper{ "RawColor", &ForwardShadingPass::rawColor,
                      TextureInterface{ .pixelFormat = PixelFormat::RGBA32Float,
                                        .size = TextureSize::ScreenSize } },
    InterfaceWrapper{
        "Depth", &ForwardShadingPass::depth,
        TextureInterface{ .pixelFormat = PixelFormat::Depth32Float,
                          .size = TextureSize::ScreenSize } }
};

ForwardShadingPass::ForwardShadingPass(std::string name):
    RenderPass(std::move(name)) {}

void ForwardShadingPass::doExecute(FrameParameters const&, RenderContext& ctx) {
    RenderPassDescription desc{};
    RenderPassColorAttachmentDescription caDesc{};
    caDesc.texture = rawColor;
    caDesc.clearColor = vml::colors<>::black;
    caDesc.loadAction = LoadAction::Clear;
    desc.colorAttachments.push_back(caDesc);
    RenderPassDepthAttachmentDescription dDesc{};
    dDesc.texture = depth;
    desc.depthAttachment = dDesc;
    ctx.begin(desc);
    ctx.setVertexBuffer(sceneParameters, 0);
    ctx.setVertexBuffer(geometry->transformBuffer(), 2);
    ctx.setFragmentBuffer(sceneParameters, 0);
    ctx.setFragmentBuffer(lights->lightBuffer(), 1);
    Material const* currentMaterial = nullptr;
    MaterialInstance const* currentMaterialInstance = nullptr;
    StaticMeshRenderer const* currentMesh = nullptr;
    for (size_t index = 0; auto&& object: geometry->getObjects()) {
        if (object.materialInstance->material() != currentMaterial) {
            currentMaterial = object.materialInstance->material();
            ctx.setPipeline(currentMaterial->rgMainPass);
            ctx.setTriangleCullMode(currentMaterial->cullMode);
            ctx.setDepthStencil(currentMaterial->depthStencil);
        }
        if (object.materialInstance.get() != currentMaterialInstance) {
            ctx.setFragmentBuffer(object.materialInstance->parameterBuffer(),
                                  3);
            currentMaterialInstance = object.materialInstance.get();
        }
        if (object.mesh.get() != currentMesh) {
            ctx.setVertexBuffer(object.mesh->vertexBuffer(), 1);
            currentMesh = object.mesh.get();
        }
        // object transform
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

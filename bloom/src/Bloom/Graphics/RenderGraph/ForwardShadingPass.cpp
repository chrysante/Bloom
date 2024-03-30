#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"

#include <vml/vml.hpp>

#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/Material/Material.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Graphics/RenderGraph/RenderScene.h"
#include "Bloom/Graphics/Renderer/ShaderParameters.h"
#include "Bloom/Graphics/StaticMesh.h"

using namespace bloom;
using namespace vml;

ForwardShadingPass::ForwardShadingPass(): RGPassNode("Forward Shading Pass") {
    declareInput(RGBufferDescription{ .name = "Scene Parameters",
                                      .handle = &sceneParameters });
    declareInput(RGSceneDescription{ .name = "Scene", .handle = &scene });

    declareOutput(
        RGTextureDescription{ .name = "Raw Color", .handle = &rawColor });
    declareOutput(RGTextureDescription{ .name = "Depth", .handle = &depth });
}

void ForwardShadingPass::doExecute(RenderContext& ctx) {
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

    // Vertex buffers
    ctx.setVertexBuffer(sceneParameters, 0,
                        offsetof(RendererParameters, scene));

    // Fragment buffers
    ctx.setFragmentBuffer(sceneParameters, 0);

    ctx.setVertexBuffer(scene->transformBuffer(), 2);
    Material const* currentMaterial = nullptr;
    MaterialInstance const* currentMaterialInstance = nullptr;
    StaticMeshRenderer const* currentMesh = nullptr;

    for (size_t index = 0; auto&& object: scene->getObjects()) {
        if (object.materialInstance->material() != currentMaterial) {
            currentMaterial = object.materialInstance->material();
            ctx.setPipeline(currentMaterial->mainPass);
            ctx.setTriangleCullMode(currentMaterial->cullMode);
            ctx.setDepthStencil(currentMaterial->depthStencil);
        }

        if (object.materialInstance.get() != currentMaterialInstance) {
            ctx.setFragmentBuffer(object.materialInstance->parameterBuffer(),
                                  2);
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

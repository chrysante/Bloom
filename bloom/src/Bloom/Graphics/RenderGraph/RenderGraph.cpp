#include "Bloom/Graphics/RenderGraph/RenderGraph.h"

using namespace bloom;

RGPassNode* RenderGraph::addPass(std::unique_ptr<RGPassNode> pass) {
    auto* result = pass.get();
    passes.push_back(std::move(pass));
    return result;
}

RGResource* RenderGraph::addResource(std::unique_ptr<RGResourceNode> node) {
    resourceNodes.push_back(std::move(node));
    resources.push_back(std::make_unique<RGResource>());
    auto* result = resources.back().get();
    result->node = resourceNodes.back().get();
    return result;
}

void RenderGraph::addLink(RGResource* input, RGResource* output) {
    BL_UNIMPLEMENTED();
}

void RenderGraph::compile() { BL_UNIMPLEMENTED(); }

void RenderGraph::execute() { BL_UNIMPLEMENTED(); }

#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/RenderScene.h"

std::unique_ptr<RenderGraph> bloom::createDefaultRenderGraph() {
    BL_UNIMPLEMENTED();
#if 0
    auto rg = std::make_unique<RenderGraph>();
    auto* scene = rg->addResource(std::make_unique<RenderScene>());
    auto* sceneParams = rg->addResource(std::make_unique<RGBufferNode>());
    auto* shadingPass = rg->addPass(std::make_unique<ForwardShadingPass>());
    rg->addLink(scene, shadingPass->findInput("Scene"));
    rg->addLink(sceneParams, shadingPass->findInput("Scene Parameters"));
    return rg;
#endif
}

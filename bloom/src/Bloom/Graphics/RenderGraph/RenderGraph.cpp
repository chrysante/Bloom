#include "Bloom/Graphics/RenderGraph/RenderGraph.h"

#include <range/v3/algorithm.hpp>

using namespace bloom;
using namespace rg;

RenderGraph::RenderGraph(HardwareDevice* device): device(device) {}

Pass* RenderGraph::addPass(std::unique_ptr<Pass> pass) {
    auto* result = pass.get();
    passes.push_back(std::move(pass));
    return result;
}

UserResource* RenderGraph::addUserResource(std::string name,
                                           ResourceHandleVariant handle) {
    resources.push_back(
        std::make_unique<UserResource>(std::move(name), std::move(handle)));
    return static_cast<UserResource*>(resources.back().get());
}

void RenderGraph::addLink(Resource* resource, PassInput* user) {
    resource->addUser(user);
    user->_resource = resource;
}

void RenderGraph::compile() { BL_UNIMPLEMENTED(); }

void RenderGraph::execute() { BL_UNIMPLEMENTED(); }

Pass* RenderGraph::findPass(std::string_view name) {
    auto itr = ranges::find_if(passes, [&](auto& pass) {
        return pass->name() == name;
    });
    return itr != passes.end() ? itr->get() : nullptr;
}

Resource* RenderGraph::findResource(std::string_view name) {
    auto itr = ranges::find_if(resources, [&](auto& resource) {
        return resource->name() == name;
    });
    return itr != resources.end() ? itr->get() : nullptr;
}

#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/Scene.h"

std::unique_ptr<RenderGraph> rg::createDefaultRenderGraph(
    HardwareDevice* device) {
    auto rg = std::make_unique<RenderGraph>(device);
    auto* geometry =
        rg->addUserResource("Geometry", std::make_unique<SceneGeometry>());
    auto* lights =
        rg->addUserResource("Lights", std::make_unique<SceneLights>());
    auto* sceneParams =
        rg->addUserResource("Parameters",
                            device->createBuffer(
                                { .size = sizeof(RendererParameters) }));
    auto* shadingPass =
        rg->addPass(std::make_unique<ForwardShadingPass>("MainPass"));
    rg->addLink(geometry, shadingPass->findInput("Geometry"));
    rg->addLink(lights, shadingPass->findInput("Lights"));
    rg->addLink(sceneParams, shadingPass->findInput("Scene Parameters"));
    rg->compile();
    return rg;
}

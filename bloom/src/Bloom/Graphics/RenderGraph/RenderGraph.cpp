#include "Bloom/Graphics/RenderGraph/RenderGraph.h"

#include <range/v3/algorithm.hpp>
#include <utl/overload.hpp>

#include "Bloom/Core/Ranges.h"

using namespace bloom;
using namespace rg;
using namespace ranges::views;

RenderGraph::RenderGraph(HardwareDevice* device): device(device) {}

Pass* RenderGraph::addPass(std::unique_ptr<Pass> pass) {
    auto* result = pass.get();
    passes.push_back(std::move(pass));
    return result;
}

void RenderGraph::addLink(Resource* from, Resource* to) {
    BL_ASSERT(!to->predecessor(), "Pass input is already linked");
    from->addSuccessor(to);
    to->setPredecessor(from);
}

static bool validate(std::span<Pass* const> passes) {
    bool result = true;
    for (auto* pass: passes) {
        for (auto* input: pass->inputs()) {
            BL_ASSERT(input->isInput());
            if (!input->predecessor()) {
                Logger::Error("Pass is not linked: ", pass->name());
                result = false;
            }
            BL_ASSERT(input->successors().empty(),
                      "Inputs cannot have successors");
        }
        for (auto* output: pass->outputs()) {
            BL_ASSERT(output->isOutput());
            BL_ASSERT(!output->predecessor(),
                      "Output cannot have predecessors");
        }
    }
    return result;
}

static utl::small_vector<Pass*> findLivePasses(std::span<Pass* const> sinks) {
    utl::hashset<Pass*> live;
    auto dfs = [&](auto& dfs, auto* pass) -> void {
        live.insert(pass);
        for (auto* input: pass->inputs()) {
            auto* pred = input->predecessor();
            if (pred && pred->parent()) {
                dfs(dfs, pred->parent());
            }
        }
    };
    for (auto* sink: sinks) {
        dfs(dfs, sink);
    }
    return live | ranges::to<utl::small_vector<Pass*>>;
}

static utl::small_vector<Pass*> findSources(std::span<Pass* const> passes) {
    return passes | filter([](Pass* pass) {
        return ranges::all_of(pass->inputs(), [](auto* input) {
            auto* pred = input->predecessor();
            BL_ASSERT(pred, "Must be set");
            return pred->parent() == nullptr;
        });
    });
}

#include <iostream>

static void print(std::string_view label, std::span<Pass* const> passes) {
    Logger::Info(label, ":");
    for (auto* pass: passes) {
        std::cout << pass->name() << std::endl;
    }
}

static std::vector<Pass*> topsort(std::span<Pass* const> passes) {
    std::vector<Pass*> order;
    order.reserve(passes.size());
    utl::hashset<Pass*> temporary, permanent;
    auto visit = [&](auto& visit, Pass* pass) -> void {
        if (permanent.contains(pass)) {
            return;
        }
        BL_ASSERT(!temporary.contains(pass), "Graph has a cycle");
        temporary.insert(pass);
        for (auto* output: pass->outputs()) {
            for (auto* succ: output->successors()) {
                BL_ASSERT(succ->parent());
                visit(visit, succ->parent());
            }
        }
        temporary.erase(pass);
        permanent.insert(pass);
        order.push_back(pass);
    };
    for (auto* pass: passes) {
        visit(visit, pass);
    }
    ranges::reverse(order);
    return order;
}

static void allocateResource(HardwareDevice& device, Resource& resource) {
    Logger::Debug("Allocating ", resource.type(), ": ", resource.name());
#if 0
    visit(resource, utl::overload{
        [&](BufferResource& buffer) {
            BL_UNIMPLEMENTED();
        },
        [&](TextureResource& resource) {
            BL_UNIMPLEMENTED();
        },
        [&](SceneGeometryResource& resource) {
            BL_UNIMPLEMENTED();
        },
        [&](SceneLightsResource& resource) {
            BL_UNIMPLEMENTED();
        },
    });
#endif
}

static void allocateResources(HardwareDevice& device,
                              std::span<Pass* const> executionOrder) {
    for (auto* pass: executionOrder) {
        for (auto* input: pass->inputs()) {
            auto* pred = input->predecessor();
            BL_ASSERT(pred);
            if (!pred->parent()) {
                allocateResource(device, *input);
            }
            else {
                visit(*input, *pred,
                      utl::overload{
                          [&]<typename T>(T const& input, T const& pred) {
                    *input.interface().handle = *pred.interface().handle;
                },
                          [](auto&, auto&) {
                    /// Type mismatch
                    BL_UNREACHABLE();
                } });
            }
        }
        for (auto* output: pass->outputs()) {
            allocateResource(device, *output);
        }
    }
}

bool RenderGraph::compile() {
    auto allPasses = passes | ToAddress<> | ToSmallVector<>;
    if (!validate(allPasses)) {
        return false;
    }
    auto sinks = allPasses | filter(&Pass::isSink) |
                 ranges::to<utl::small_vector<Pass*>>;
    auto livePasses = findLivePasses(sinks);
    executionOrder = topsort(livePasses);
    allocateResources(*device, executionOrder);

    return true;
}

void RenderGraph::execute() { BL_UNIMPLEMENTED(); }

Pass* RenderGraph::findPass(std::string_view name) {
    auto itr = ranges::find_if(passes, [&](auto& pass) {
        return pass->name() == name;
    });
    return itr != passes.end() ? itr->get() : nullptr;
}

Resource* RenderGraph::findInputResource(std::string_view name) {
    auto itr = ranges::find_if(inputs, [&](auto& resource) {
        return resource->name() == name;
    });
    return itr != inputs.end() ? itr->get() : nullptr;
}

#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/Scene.h"

std::unique_ptr<RenderGraph> rg::createDefaultRenderGraph(
    HardwareDevice* device) {
    auto rg = std::make_unique<RenderGraph>(device);
    auto* geometry =
        rg->declareInput(SceneGeometryInterface{ .name = "Geometry" });
    auto* lights = rg->declareInput(SceneLightsInterface{ .name = "Lights" });
    auto* sceneParams =
        rg->declareInput(BufferInterface{ .name = "Parameters" });
    auto* shadingPass =
        rg->addPass(std::make_unique<ForwardShadingPass>("MainPass"));
    shadingPass->markSink();
    rg->addLink(geometry, shadingPass->findInput("Geometry"));
    rg->addLink(lights, shadingPass->findInput("Lights"));
    rg->addLink(sceneParams, shadingPass->findInput("Scene Parameters"));
    rg->compile();
    return rg;
}

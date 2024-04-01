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

void RenderGraph::addLink(PassSource* source, PassSink* sink) {
    BL_ASSERT(!sink->source(), "Pass sink is already linked");
    source->addSink(sink);
    sink->setSource(source);
}

static bool validate(std::span<Pass* const> passes) {
    bool result = true;
    for (auto* pass: passes) {
        for (auto* sink: pass->sinks()) {
            if (!sink->source()) {
                Logger::Error("Sink is not linked: ", pass->name(), ".",
                              sink->name());
                result = false;
            }
        }
    }
    return result;
}

static utl::small_vector<Pass*> findLivePasses(
    std::span<Pass* const> sinkPasses) {
    struct DFS {
        utl::hashset<Pass*> live;
        void search(Pass* pass) {
            live.insert(pass);
            for (auto* sink: pass->sinks()) {
                if (auto* sourcePass = sink->getSourcePass()) {
                    search(sourcePass);
                }
            }
        }
    };
    DFS dfs;
    for (auto* sinkPass: sinkPasses) {
        dfs.search(sinkPass);
    }
    return dfs.live | ToSmallVector<>;
}

static utl::small_vector<Pass*> findSources(std::span<Pass* const> passes) {
    return passes | filter([](Pass* pass) {
        return ranges::all_of(pass->sinks(), [](auto* sink) {
            return sink->getSourcePass() == nullptr;
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
    auto dfs = [&](auto& dfs, Pass* pass) -> void {
        BL_EXPECT(pass);
        if (permanent.contains(pass)) {
            return;
        }
        BL_ASSERT(!temporary.contains(pass), "Graph has a cycle");
        temporary.insert(pass);
        for (auto* source: pass->sources()) {
            for (auto* user: source->getSinkPasses()) {
                dfs(dfs, user);
            }
        }
        temporary.erase(pass);
        permanent.insert(pass);
        order.push_back(pass);
    };
    for (auto* pass: passes) {
        dfs(dfs, pass);
    }
    ranges::reverse(order);
    return order;
}

static void allocateResource(HardwareDevice& device, ResourcePool& pool,
                             PassInterfaceBase& interface) {
    auto* resource = interface.visit([&]<typename I>(I const&) -> Resource* {
        return pool.allocate<typename ToResourceType<I>::type>();
    });
    resource->addUser(&interface);
}

static void allocateResources(HardwareDevice& device, ResourcePool& pool,
                              std::span<Pass* const> executionOrder) {
    for (auto* pass: executionOrder) {
        for (auto* sink: pass->sinks()) {
            auto* sourcePass = sink->getSourcePass();
            if (!sourcePass) {
                allocateResource(device, pool, *sink);
                continue;
            }
            BL_ASSERT(sink->source(), "Sink not linked");
            auto* resource = sink->source()->resource();
            BL_ASSERT(resource, "Source has no resource assigned");
            sink->setResource(resource);
            resource->addUser(sink);
        }
        for (auto* source: pass->sources()) {
            allocateResource(device, pool, *source);
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
    allocateResources(*device, pool, executionOrder);

    return true;
}

void RenderGraph::execute() { BL_UNIMPLEMENTED(); }

Pass* RenderGraph::findPass(std::string_view name) {
    auto itr = ranges::find_if(passes, [&](auto& pass) {
        return pass->name() == name;
    });
    return itr != passes.end() ? itr->get() : nullptr;
}

PassSource* RenderGraph::findInputResource(std::string_view name) {
    auto itr = ranges::find_if(inputs, [&](auto& resource) {
        return resource->name() == name;
    });
    return itr != inputs.end() ? itr->get() : nullptr;
}

#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"

std::unique_ptr<RenderGraph> rg::createDefaultRenderGraph(
    HardwareDevice* device) {
    auto rg = std::make_unique<RenderGraph>(device);
    auto* geometry = rg->declareInput(GeometryInterface{ .name = "Geometry" });
    auto* lights = rg->declareInput(LightsInterface{ .name = "Lights" });
    auto* sceneParams =
        rg->declareInput(BufferInterface{ .name = "Parameters" });
    auto* shadingPass =
        rg->addPass(std::make_unique<ForwardShadingPass>("MainPass"));
    shadingPass->markSink();
    rg->addLink(geometry, shadingPass->findSink("Geometry"));
    rg->addLink(lights, shadingPass->findSink("Lights"));
    rg->addLink(sceneParams, shadingPass->findSink("Scene Parameters"));
    rg->compile();
    return rg;
}

#include "Bloom/Graphics/RenderGraph/RenderGraph.h"

#include <range/v3/algorithm.hpp>
#include <utl/overload.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;
using namespace rg;
using namespace ranges::views;

RenderGraph::RenderGraph(std::string name, RenderGraphContext& ctx,
                         HardwareDevice& device):
    Pass(std::move(name), &device), ctx(&ctx) {}

Pass* RenderGraph::addPass(std::unique_ptr<Pass> pass) {
    auto* result = pass.get();
    result->_parent = this;
    result->_device = &device();
    passes.push_back(std::move(pass));
    return result;
}

void RenderGraph::addLink(PassSource* source, PassSink* sink) {
    BL_EXPECT(source);
    BL_EXPECT(sink);
    BL_ASSERT(!sink->source(), "Pass sink is already linked");
    source->addSink(sink);
    sink->setSource(source);
}

static bool validate(std::span<Pass* const> passes) {
    bool result = true;
    for (auto* pass: passes) {
        for (auto* sink: pass->sinks()) {
            if (!sink->isOptional() && !sink->source()) {
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

static std::vector<Pass*> topsort(std::span<Pass* const> passes) {
    std::vector<Pass*> order;
    order.reserve(passes.size());
    utl::hashset<Pass*> temporary, permanent;
    auto dfs = [&](auto& dfs, Pass* pass) -> void {
        if (!pass || permanent.contains(pass)) {
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

static void allocateBackingResource(ResourcePool& pool,
                                    PassInterfaceBase& interface) {
    auto* resource = interface.visit(
        [&]<InterfaceType Type>(InterfaceDesc<Type> const& desc) -> Resource* {
        return pool.allocate(desc);
    });
    resource->addUser(&interface);
    interface.setResource(resource);
}

static void allocateBackingResources(
    ResourcePool& pool, std::span<Pass* const> executionOrder,
    std::span<RenderGraphSource* const> outputs) {
    for (auto* pass: executionOrder) {
        for (auto* sink: pass->activeSinks()) {
            auto* source = sink->source();
            if (!source->pass()) {
                allocateBackingResource(pool, *source);
            }
            auto* resource = source->resource();
            BL_ASSERT(resource, "Source has no resource assigned");
            sink->setResource(resource);
            resource->addUser(sink);
        }
        for (auto* source: pass->sources()) {
            allocateBackingResource(pool, *source);
        }
    }
    for (auto* output: outputs) {
        if (auto* source = output->internalSink()->source()) {
            output->setResource(source->resource());
        }
    }
    for (auto* resource: pool.resources()) {
        resource->assignUsers();
    }
}

static bool isSinkPass(Pass const* pass) {
    auto targets = pass->sources() |
                   transform([](auto* source) { return source->sinks(); }) |
                   join;
    return ranges::any_of(targets, [](auto* sink) {
        return dynamic_cast<RenderGraphInternalSink const*>(sink);
    });
}

bool RenderGraph::compile() {
    auto allPasses = passes | ToAddress<> | ToSmallVector<>;
    if (!validate(allPasses)) {
        return false;
    }
    auto sinkPasses = allPasses | filter(isSinkPass) |
                      ranges::to<utl::small_vector<Pass*>>;
    auto livePasses = findLivePasses(sinkPasses);
    executionOrder = topsort(livePasses);
    auto outputs = sources() | transform([](PassSource* source) {
        return dynamic_cast<RenderGraphSource*>(source);
    }) | ranges::to<utl::small_vector<RenderGraphSource*>>;
    allocateBackingResources(context().resourcePool, executionOrder, outputs);
    return true;
}

static void reallocateHardwareResource(HardwareDevice& device,
                                       BufferResource& resource,
                                       FrameParameters const&) {
    resource.assign(device.createBuffer({
        .size = resource.desc().size,
        .storageMode = StorageMode::Managed // For now!
    }));
}

static void reallocateHardwareResource(HardwareDevice& device,
                                       TextureResource& resource,
                                       FrameParameters const& params) {
    auto const& desc = resource.desc();
    resource.assign(device.createTexture(
        { .type = desc.type,
          .pixelFormat = desc.pixelFormat,
          .size = desc.size.compute(params.screenSize),
          .mipmapLevelCount = desc.mipmapLevelCount,
          .sampleCount = desc.sampleCount,
          .arrayLength = desc.arrayLength,
          .usage = TextureUsage::ShaderRead | TextureUsage::RenderTarget }));
}

static void reallocateHardwareResource(HardwareDevice&, GeometryResource&,
                                       FrameParameters const&) {}

static void reallocateHardwareResource(HardwareDevice&, LightsResource&,
                                       FrameParameters const&) {}

static void reallocateHardwareResources(HardwareDevice& device,
                                        ResourcePool& pool,
                                        std::span<Pass* const> passes,
                                        FrameParameters const& frameParams) {
    for (auto* resource: pool.resources()) {
        visit(*resource, [&](auto& resource) {
            reallocateHardwareResource(device, resource, frameParams);
        });
    }
    for (auto* pass: passes) {
        pass->allocateResources(frameParams, device);
    }
}

void RenderGraph::allocateResources(FrameParameters params) {
    if (lastFrameParams == params) {
        return;
    }
    reallocateHardwareResources(device(), context().resourcePool,
                                executionOrder, params);
    lastFrameParams = params;
}

Pass* RenderGraph::findPass(std::string_view name) {
    auto itr = ranges::find_if(passes, [&](auto& pass) {
        return pass->name() == name;
    });
    return itr != passes.end() ? itr->get() : nullptr;
}

PassSource* RenderGraph::findInputSource(std::string_view name) {
    auto sinks = this->sinks();
    auto itr = ranges::find_if(sinks, [&](auto* sink) {
        return sink->name() == name;
    });
    if (itr == sinks.end()) {
        return nullptr;
    }
    auto* sink = dynamic_cast<RenderGraphSink*>(*itr);
    BL_ASSERT(sink);
    return sink->internalSource();
}

void RenderGraph::doExecute(FrameParameters const& params,
                            CommandQueue& cmdQueue) {
    for (auto* pass: executionOrder) {
        pass->execute(params, cmdQueue);
    }
}

static std::unique_ptr<RenderGraph> gRenderGraph;
static RenderGraphContext gRenderGraphCtx;

RenderGraph* RenderGraphManager::get() { return gRenderGraph.get(); }

void RenderGraphManager::set(std::unique_ptr<RenderGraph> rg) {
    gRenderGraph = std::move(rg);
}

RenderGraphContext& RenderGraphManager::context() { return gRenderGraphCtx; }

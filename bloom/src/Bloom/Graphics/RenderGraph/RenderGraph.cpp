#include "Bloom/Graphics/RenderGraph/RenderGraph.h"

#include <range/v3/algorithm.hpp>
#include <utl/overload.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;
using namespace ranges::views;

RenderGraph::RenderGraph(std::string name, RenderGraphContext& ctx,
                         HardwareDevice& device):
    Pass(std::move(name), &device), ctx(&ctx) {}

Pass* RenderGraph::addPass(std::unique_ptr<Pass> pass) {
    BL_EXPECT(!isCompiled());
    auto* result = pass.get();
    result->_parent = this;
    result->_device = &device();
    passes.push_back(std::move(pass));
    return result;
}

void RenderGraph::addLink(PassSource* source, PassSink* sink) {
    BL_EXPECT(!isCompiled());
    BL_EXPECT(source);
    BL_EXPECT(sink);
    if (sink->source()) {
        Logger::Error("Pass sink is already linked: ", sink->qualifiedName());
        return;
    }
    source->addSink(sink);
    sink->setSource(source);
}

static std::string format(InterfaceType type) {
    using enum InterfaceType;
    switch (type) {
    case BufferInterface:
        return "buffer";
    case TextureInterface:
        return "texture";
    case GeometryInterface:
        return "geometry";
    case LightsInterface:
        return "lights";
    default:
        return "invalid type";
    }
}

static bool typecheck(PassSource const& source, PassSink const& sink) {
    auto sinkType = sink.interface().base().type();
    auto sourceType = source.interface().base().type();
    if (sinkType != sourceType) {
        Logger::Error("Type mismatch on sink \"", sink.qualifiedName(), "\". ",
                      "Have ", format(sourceType), ", expected ",
                      format(sinkType));
        return false;
    }
    return true;
}

static bool validateSink(PassSink const& sink) {
    auto* source = sink.source();
    if (!source) {
        if (sink.isOptional()) {
            return true;
        }
        Logger::Error("Sink is not linked: ", sink.qualifiedName());
        return false;
    }
    if (!typecheck(*source, sink)) {
        return false;
    }
    return true;
}

static bool validate(std::span<Pass* const> passes) {
    bool result = true;
    for (auto* pass: passes) {
        for (auto* sink: pass->sinks()) {
            BL_ASSERT(sink);
            result &= validateSink(*sink);
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

/// \Returns `true` if \p pass is linked to a source of the enclosing render
/// graph
static bool isSinkPass(Pass const* pass) {
    auto targets = pass->sources() |
                   transform([](auto* source) { return source->sinks(); }) |
                   join;
    return ranges::any_of(targets, [](auto* sink) -> bool {
        return dynamic_cast<RenderGraphInternalSink const*>(sink);
    });
}

bool RenderGraph::compile() {
    /// We assert this for now until we fiugre out what to do in this case
    BL_ASSERT(!isCompiled());
    auto allPasses = passes | ToAddress<> | ToSmallVector<>;
    if (!validate(allPasses)) {
        return false;
    }
    auto sinkPasses = allPasses | filter(isSinkPass) |
                      ranges::to<utl::small_vector<Pass*>>;
    auto livePasses = findLivePasses(sinkPasses);
    executionOrder = topsort(livePasses);
    for (auto* pass: executionOrder) {
        auto* rg = dynamic_cast<RenderGraph*>(pass);
        if (rg && !rg->isCompiled()) {
            rg->compile();
        }
    }
    return true;
}

static void allocateBackingResource(ResourcePool& pool,
                                    PassTerminal& terminal) {
    auto* resource = pool.allocate(terminal.interface());
    resource->addUser(&terminal);
    terminal.setResource(resource);
}

void RenderGraph::allocateBackingResources() {
    for (auto* pass: executionOrder) {
        for (auto* sink: pass->activeSinks()) {
            auto* source = sink->source();
            /// Here we lazily allocate resources for graph inputs
            if (!source->resource() && !source->pass()) {
                allocateBackingResource(context().resourcePool, *source);
            }
            auto* resource = source->resource();
            BL_ASSERT(resource, "Source has no resource assigned");
            sink->setResource(resource);
            resource->addUser(sink);
        }
        for (auto* source: pass->sources()) {
            allocateBackingResource(context().resourcePool, *source);
        }
        if (auto* rg = dynamic_cast<RenderGraph*>(pass)) {
            rg->allocateBackingResources();
        }
    }
    auto outputs = sources() | transform([](PassSource* source) {
        return dynamic_cast<RenderGraphSource*>(source);
    }) | ranges::to<utl::small_vector<RenderGraphSource*>>;
    for (auto* output: outputs) {
        if (auto* source = output->internalSink()->source()) {
            output->setResource(source->resource());
        }
    }
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

PassSource* RenderGraph::inputSource(size_t index) {
    return dynamic_cast<RenderGraphSink*>(sink(index))->internalSource();
}

void RenderGraph::doExecute(FrameParameters const& params,
                            CommandQueue& cmdQueue) {
    for (auto* pass: executionOrder) {
        pass->execute(params, cmdQueue);
    }
}

void RenderGraph::doAllocateResources(FrameParameters const& params,
                                      HardwareDevice& device) {
    if (lastFrameParams == params) {
        return;
    }
    if (!parent()) {
        for (auto* resource: context().resourcePool.resources()) {
            resource->reallocate(device, params);
        }
    }
    for (auto* pass: executionOrder) {
        pass->allocateResources(params, device);
    }
    lastFrameParams = params;
}

std::unique_ptr<RenderGraph> RenderGraph::clone(
    RenderGraphContext& context) const {
    auto p = Pass::clone(context);
    return std::unique_ptr<RenderGraph>(
        dynamic_cast<RenderGraph*>(p.release()));
}

Pass* get(utl::hashmap<Pass const*, Pass*> const& map, Pass const* pass) {
    auto itr = map.find(pass);
    BL_ASSERT(itr != map.end());
    return itr->second;
}

std::unique_ptr<Pass> RenderGraph::doClone(RenderGraphContext& context) const {
    auto clone = std::make_unique<RenderGraph>(name(), context, device());
    for (auto* sink: sinks()) {
        clone->declareSink(sink->interface());
    }
    for (auto* source: sources()) {
        clone->declareSource(source->interface());
    }
    utl::hashmap<Pass const*, Pass*> map;
    for (auto& pass: passes) {
        auto* clonedPass = clone->addPass(pass->clone(context));
        clonedPass->_device = &device();
        clonedPass->_parent = clone.get();
        map[pass.get()] = clonedPass;
    }
    auto cloneSinkLinks = [&](PassSink const* sink, PassSink* cloneSink) {
        auto* source = sink->source();
        if (!source) {
            return;
        }
        if (source->pass()) {
            auto* sourcePass = get(map, source->pass());
            clone->addLink(sourcePass->source(source->indexInParent()),
                           cloneSink);
            return;
        }
        auto* internalSource =
            dynamic_cast<RenderGraphInternalSource const*>(source);
        if (!internalSource) {
            return;
        }
        BL_ASSERT(internalSource->owningSink().pass() == this);
        size_t sourceIndex = internalSource->owningSink().indexInParent();
        clone->addLink(clone->inputSource(sourceIndex), cloneSink);
    };
    for (auto& pass: passes) {
        auto* clonedPass = get(map, pass.get());
        for (auto [index, sink]: pass->sinks() | enumerate) {
            cloneSinkLinks(sink, clonedPass->sink(index));
        }
    }
    for (auto [source, cloneSource]: zip(sources(), clone->sources())) {
        auto* sink =
            static_cast<RenderGraphSource const*>(source)->internalSink();
        auto* cloneSink =
            static_cast<RenderGraphSource*>(cloneSource)->internalSink();
        cloneSinkLinks(sink, cloneSink);
    }
    clone->executionOrder = executionOrder | transform([&](auto* pass) {
        return get(map, pass);
    }) | ranges::to<std::vector>;
    return clone;
}

static std::unique_ptr<RenderGraph> gRenderGraph;
static RenderGraphContext gRenderGraphCtx;

RenderGraph* RenderGraphManager::get() { return gRenderGraph.get(); }

void RenderGraphManager::set(std::unique_ptr<RenderGraph> rg) {
    gRenderGraph = std::move(rg);
}

RenderGraphContext& RenderGraphManager::context() { return gRenderGraphCtx; }

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
    Pass(std::move(name)), ctx(&ctx), mDevice(&device) {}

Pass* RenderGraph::addPass(std::unique_ptr<Pass> pass) {
    auto* result = pass.get();
    result->_parent = this;
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

static void allocateBackingResource(ResourcePool& pool,
                                    PassInterfaceBase& interface) {
    auto* resource = interface.visit([&]<typename I>(I const&) -> Resource* {
        return pool.allocate(interface.get<I>());
    });
    resource->addUser(&interface);
    interface.setResource(resource);
}

static void allocateBackingResources(ResourcePool& pool,
                                     std::span<Pass* const> executionOrder) {
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
    for (auto* resource: pool.resources()) {
        resource->assignUsers();
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
    allocateBackingResources(context().resourcePool, executionOrder);
    return true;
}

static void reallocateHardwareResource(HardwareDevice& device,
                                       BufferResource& resource,
                                       FrameParameters const&) {
    resource.assign(device.createBuffer({
        .size = resource.interface().size,
        .storageMode = StorageMode::Managed // For now!
    }));
}

static void reallocateHardwareResource(HardwareDevice& device,
                                       TextureResource& resource,
                                       FrameParameters const& params) {
    auto const& interface = resource.interface();
    resource.assign(device.createTexture(
        { .type = interface.type,
          .pixelFormat = interface.pixelFormat,
          .size = interface.size.compute(params.screenSize),
          .mipmapLevelCount = interface.mipmapLevelCount,
          .sampleCount = interface.sampleCount,
          .arrayLength = interface.arrayLength,
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
    return sink->source();
}

void RenderGraph::doExecute(FrameParameters const& params,
                            CommandQueue& cmdQueue) {
    for (auto* pass: executionOrder) {
        pass->execute(params, cmdQueue);
    }
}

#include "Bloom/Graphics/RenderGraph/BloomPass.h"
#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/PostProcessPass.h"
#include "Bloom/Shader/PostProcessing.h"

static std::unique_ptr<RenderGraph> createDefaultRenderGraph(
    RenderGraphContext& ctx, HardwareDevice& device) {
    auto rg = std::make_unique<RenderGraph>("DefaultRenderGraph", ctx, device);
    auto* geometry = rg->declareSink("Geometry", GeometryInterface{});
    auto* lights = rg->declareSink("Lights", LightsInterface{});
    auto* sceneParams =
        rg->declareSink("SceneParameters",
                        BufferInterface{ .size =
                                             sizeof(SceneRenderParameters) });
    auto* bloomParams =
        rg->declareSink("BloomParameters",
                        BufferInterface{ .size = sizeof(BloomParameters) });
    auto* postprocessParams =
        rg->declareSink("PostProcessParameters",
                        BufferInterface{ .size =
                                             sizeof(PostProcessParameters) });
    auto* shadingPass =
        rg->addPass(std::make_unique<ForwardShadingPass>("MainPass"));
    auto* bloomPass = rg->addPass(std::make_unique<BloomPass>("Bloom"));
    auto* postProcessPass =
        rg->addPass(std::make_unique<PostProcessPass>("PostProcess"));
    postProcessPass->markSink();
    auto* rawColor = shadingPass->findSource("RawColor");
    auto* bloomColor = bloomPass->findSource("Output");
    /// Shading pass
    rg->addLink(geometry->internalSource(), shadingPass->findSink("Geometry"));
    rg->addLink(lights->internalSource(), shadingPass->findSink("Lights"));
    rg->addLink(sceneParams->internalSource(),
                shadingPass->findSink("SceneParameters"));
    /// Bloom pass
    rg->addLink(bloomParams->internalSource(),
                bloomPass->findSink("Parameters"));
    rg->addLink(rawColor, bloomPass->findSink("RawColor"));
    /// Post process pass
    rg->addLink(postprocessParams->internalSource(),
                postProcessPass->findSink("Parameters"));
    //    rg->addLink(bloomParams->internalSource(),
    //                postProcessPass->findSink("BloomParameters"));
    rg->addLink(rawColor, postProcessPass->findSink("RawColor"));
    //    rg->addLink(bloomColor, postProcessPass->findSink("BloomColor"));
    rg->compile();
    return rg;
}

RenderGraph* rg::getGlobalRenderGraph(HardwareDevice* device) {
    static RenderGraphContext ctx;
    static auto instance = createDefaultRenderGraph(ctx, *device);
    return instance.get();
}

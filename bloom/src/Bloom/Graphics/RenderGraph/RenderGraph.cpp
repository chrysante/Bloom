#include "Bloom/Graphics/RenderGraph/RenderGraph.h"

#include <range/v3/algorithm.hpp>
#include <utl/overload.hpp>

#include "Bloom/Core/Ranges.h"
#include "Bloom/GPU/HardwareDevice.h"

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
        for (auto* sink: pass->sinks()) {
            auto* source = sink->source();
            BL_ASSERT(source, "Sink not linked");
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
    allocateBackingResources(pool, executionOrder);
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
    reallocateHardwareResources(*device, pool, executionOrder, params);
    lastFrameParams = params;
}

void RenderGraph::execute(FrameParameters params, CommandQueue& cmdQueue) {
    for (auto* pass: executionOrder) {
        pass->execute(params, cmdQueue);
    }
}

Pass* RenderGraph::findPass(std::string_view name) {
    auto itr = ranges::find_if(passes, [&](auto& pass) {
        return pass->name() == name;
    });
    return itr != passes.end() ? itr->get() : nullptr;
}

PassSource* RenderGraph::findInputSource(std::string_view name) {
    auto itr = ranges::find_if(inputs, [&](auto& source) {
        return source->name() == name;
    });
    return itr != inputs.end() ? itr->get() : nullptr;
}

#include "Bloom/Graphics/RenderGraph/BloomPass.h"
#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/PostProcessPass.h"
#include "Bloom/Shader/PostProcessing.h"

std::unique_ptr<RenderGraph> rg::createDefaultRenderGraph(
    HardwareDevice* device) {
    auto rg = std::make_unique<RenderGraph>(device);
    auto* geometry = rg->declareInput("Geometry", GeometryInterface{});
    auto* lights = rg->declareInput("Lights", LightsInterface{});
    auto* sceneParams =
        rg->declareInput("SceneParameters",
                         BufferInterface{ .size =
                                              sizeof(SceneRenderParameters) });
    auto* bloomParams =
        rg->declareInput("BloomParameters",
                         BufferInterface{ .size = sizeof(BloomParameters) });
    auto* postprocessParams =
        rg->declareInput("PostProcessParameters",
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
    rg->addLink(geometry, shadingPass->findSink("Geometry"));
    rg->addLink(lights, shadingPass->findSink("Lights"));
    rg->addLink(sceneParams, shadingPass->findSink("SceneParameters"));
    /// Bloom pass
    rg->addLink(bloomParams, bloomPass->findSink("Parameters"));
    rg->addLink(rawColor, bloomPass->findSink("RawColor"));
    /// Post process pass
    rg->addLink(postprocessParams, postProcessPass->findSink("Parameters"));
    rg->addLink(bloomParams, postProcessPass->findSink("BloomParameters"));
    rg->addLink(rawColor, postProcessPass->findSink("RawColor"));
    rg->addLink(bloomColor, postProcessPass->findSink("BloomColor"));
    rg->compile();
    return rg;
}

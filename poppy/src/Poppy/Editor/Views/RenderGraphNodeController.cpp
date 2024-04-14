#include "Poppy/Editor/Views/RenderGraphNodeController.h"

#include <vml/vml.hpp>

#include "Bloom/Graphics/RenderGraph/BloomPass.h"
#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/PostProcessPass.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Shader/PostProcessing.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

using Entry = PassRegistry::Entry;

static PassRegistry& gRegistry() {
    static PassRegistry instance;
    return instance;
}

PassRegistry const& PassRegistry::get() { return gRegistry(); }

void poppy::registerPass(Entry entry) {
    gRegistry().entries.push_back(std::move(entry));
}

#define REGISTER_PASS(...)                                                     \
    UTL_STATIC_INIT { registerPass(Entry __VA_ARGS__); };

static constexpr float4 InputNodeColor = { .2, 1, .1, 1 };
static constexpr float4 OutputNodeColor = { 1, .2, .1, 1 };
static constexpr float4 PassNodeColor = { .1, .2, 1, 1 };

namespace {

struct GlobalInputNodeController: RenderGraphNodeController {
    void create(RenderGraph& rg) override {
        declare(rg, "Geometry", GeometryInterface{});
        declare(rg, "Lights", LightsInterface{});
        declare(rg, "SceneParameters",
                BufferInterface{ .size = sizeof(SceneRenderParameters) });
        declare(rg, "BloomParameters",
                BufferInterface{ .size = sizeof(BloomParameters) });
        declare(rg, "PostProcessParameters",
                BufferInterface{ .size = sizeof(PostProcessParameters) });
    }

    PassSource* getSource(size_t index) const override {
        BL_ASSERT(index < sources.size());
        return sources[index];
    }

    PassSink* getSink([[maybe_unused]] size_t index) const override {
        BL_UNREACHABLE();
    }

    void declare(RenderGraph& rg, std::string name, auto interface) {
        RenderGraphSink* sink =
            rg.declareSink(std::move(name), std::move(interface));
        sources.push_back(sink->internalSource());
    }

    std::vector<PassSource*> sources;
};

REGISTER_PASS({ .name = "Input", .create = [] {
                   return NodeDesc {
            .name = "Input",
            .outputs = {
                { .name = "Geometry" },
                { .name = "Lights" },
                { .name = "Bloom Parameters" },
                { .name = "Post Process Parameters" },
            },
                .color = InputNodeColor
        };
               } })

struct GlobalOutputNodeController: RenderGraphNodeController {
    void create(RenderGraph& rg) override {
        declare(rg, "Color",
                TextureInterface{
                    .pixelFormat = PixelFormat::RGBA8Unorm,
                    .size = TextureSize::ScreenSize,
                });
    }

    PassSource* getSource([[maybe_unused]] size_t index) const override {
        BL_UNREACHABLE();
    }

    PassSink* getSink(size_t index) const override {
        BL_ASSERT(index < sinks.size());
        return sinks[index];
    }

    void declare(RenderGraph& rg, std::string name, auto interface) {
        RenderGraphSource* source =
            rg.declareSource(std::move(name), std::move(interface));
        sinks.push_back(source->internalSink());
    }

    std::vector<PassSink*> sinks;
};

REGISTER_PASS({ .name = "Output", .create = [] {
                   return NodeDesc {
            .name = "Output",
            .inputs = {
                { .name = "Color" },
            },
            .color = OutputNodeColor
        };
               } })

struct PassNodeController: RenderGraphNodeController {
    virtual Pass* createPass(RenderGraph& renderGraph) = 0;

    void create(RenderGraph& rg) final { pass = createPass(rg); }

    PassSource* getSource(size_t index) const final {
        return pass->source(index);
    }

    PassSink* getSink(size_t index) const final { return pass->sink(index); }

    Pass* pass;
};

struct ForwardShadingPassNodeController: PassNodeController {
    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<ForwardShadingPass>("MainPass"));
    }
};

REGISTER_PASS({ .name = "Forward Shading Pass", .create = [] {
                   return NodeDesc {
            .name = "Forward Shading Pass",
            .inputs = {
                { .name = "Scene Parameters" },
                { .name = "Geometry" },
                { .name = "Lights" },
            },
            .outputs = {
                { .name = "Raw Color" },
                { .name = "Depth" },
            },
            .color = PassNodeColor
        };
               } })

} // namespace

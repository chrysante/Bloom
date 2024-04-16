#include "Poppy/Editor/Views/RenderGraphNodeController.h"

#include <utl/common.hpp>
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
using namespace ranges::views;

static constexpr float4 InputNodeColor = { 0.4, 0.7, 0.2, 1.0 };
static constexpr float4 PassNodeColor = { 0.3, 0.0, 0.8, 1.0 };
static constexpr float4 OutputNodeColor = { 1.0, 0.0, 0.2, 1.0 };
static constexpr float2 MinSize = { 100, 0 };

using Entry = PassRegistry::Entry;

static PassRegistry& gRegistry() {
    static PassRegistry instance;
    return instance;
}

PassRegistry const& PassRegistry::get() { return gRegistry(); }

Entry& poppy::registerPass() {
    auto& entries = gRegistry().entries;
    entries.emplace_back();
    return entries.back();
}

static PinFlags translateFlags(InterfaceFlags flags) {
    PinFlags result{};
    if (test(flags & InterfaceFlags::Optional)) {
        result |= PinFlags::Optional;
    }
    return result;
}

static PinDesc interfaceToPinDesc(auto const& interface) {
    // clang-format off
    return std::visit([&](auto& interface) {
        return PinDesc {
            .name = interface.name,
            .flags = translateFlags(interface.flags)
        };
    }, interface); // clang-format on
}

template <typename PassType>
static void registerConcretePass(std::string name) {
    auto& desc = registerPass().desc;
    desc.name = name;
    desc.inputs = PassType::Sinks() | transform([](auto& interface) {
        return interfaceToPinDesc(interface);
    }) | ranges::to<std::vector>;
    desc.outputs = PassType::Sources() | transform([](auto& interface) {
        return interfaceToPinDesc(interface);
    }) | ranges::to<std::vector>;
    desc.color = PassNodeColor;
    desc.minSize = MinSize;
}

#define REGISTER_PASS                                                          \
    static auto const& UTL_UNIQUE_NAME(BL_Pass_) = registerPass().desc =       \
        NodeDesc

#define AUTOREGISTER_PASS(PassType)                                            \
    UTL_STATIC_INIT { registerConcretePass<PassType>(#PassType); }

namespace {

struct GlobalInputNodeController: RenderGraphNodeController {
    void create(RenderGraph& rg) override {
        using enum InterfaceType;
        declare<Geometry>(rg, "Geometry", {});
        declare<Lights>(rg, "Lights", {});
        declare<Buffer>(rg, "SceneParameters",
                        { .size = sizeof(SceneRenderParameters) });
        declare<Buffer>(rg, "BloomParameters",
                        { .size = sizeof(BloomParameters) });
        declare<Buffer>(rg, "PostProcessParameters",
                        { .size = sizeof(PostProcessParameters) });
    }

    PassSource* getSource(size_t index) const override {
        BL_ASSERT(index < sources.size());
        return sources[index];
    }

    PassSink* getSink([[maybe_unused]] size_t index) const override {
        BL_UNREACHABLE();
    }

    template <InterfaceType Type>
    void declare(RenderGraph& rg, std::string name, InterfaceDesc<Type> desc) {
        RenderGraphSink* sink = rg.declareSink<Type>(std::move(name), desc);
        sources.push_back(sink->internalSource());
    }

    std::vector<PassSource*> sources;
};

REGISTER_PASS {
    .name = "Input",
    .outputs = {
        { .name = "Geometry" },
        { .name = "Lights" },
        { .name = "Bloom Parameters" },
        { .name = "Post Process Parameters" },
    },
    .color = InputNodeColor,
    .minSize = MinSize
};

struct GlobalOutputNodeController: RenderGraphNodeController {
    void create(RenderGraph& rg) override {
        using enum InterfaceType;
        declare<Texture>(rg, "Color",
                         {
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

    template <InterfaceType Type>
    void declare(RenderGraph& rg, std::string name, InterfaceDesc<Type> desc) {
        RenderGraphSource* source =
            rg.declareSource<Type>(std::move(name), desc);
        sinks.push_back(source->internalSink());
    }

    std::vector<PassSink*> sinks;
};

REGISTER_PASS {
    .name = "Output",
    .inputs = { { .name = "Color" }, },
    .color = OutputNodeColor,
    .minSize = MinSize
};

struct PassNodeController: RenderGraphNodeController {
    virtual Pass* createPass(RenderGraph& renderGraph) = 0;

    void create(RenderGraph& rg) final { pass = createPass(rg); }

    PassSource* getSource(size_t index) const final {
        return pass->source(index);
    }

    PassSink* getSink(size_t index) const final { return pass->sink(index); }

    Pass* pass;
};

struct ForwardShadingPassController: PassNodeController {
    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<ForwardShadingPass>("MainPass"));
    }
};

AUTOREGISTER_PASS(ForwardShadingPass);

struct PostProcessPassController: PassNodeController {
    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<PostProcessPass>("PostProcessPass"));
    }
};

AUTOREGISTER_PASS(PostProcessPass);

struct BloomController: PassNodeController {
    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<BloomPass>("BloomPass"));
    }
};

AUTOREGISTER_PASS(BloomPass);

} // namespace

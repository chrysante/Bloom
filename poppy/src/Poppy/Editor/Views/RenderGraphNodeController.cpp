#include "Poppy/Editor/Views/RenderGraphNodeController.h"

#include <utl/common.hpp>
#include <utl/strcat.hpp>
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

template <typename PassType, typename ControllerType>
static void registerConcretePass(std::string name) {
    auto& entry = registerPass();
    entry.name = name;
    entry.factory = [=] {
        return NodeDesc{
            .name = name,
            .minSize = MinSize,
            .inputs = PassType::Sinks() | transform([](auto& interface) {
            return interfaceToPinDesc(interface);
        }) | ranges::to<std::vector>,
            .outputs = PassType::Sources() | transform([](auto& interface) {
            return interfaceToPinDesc(interface);
        }) | ranges::to<std::vector>,
            .color = PassNodeColor,
            .controller = std::make_unique<ControllerType>(),
        };
    };
}

#define REGISTER_PASS(Name, ...)                                               \
    UTL_STATIC_INIT {                                                          \
        auto& entry = registerPass();                                          \
        entry.name = Name;                                                     \
        entry.factory = [=] { return NodeDesc __VA_ARGS__; };                  \
    }

#define AUTOREGISTER_PASS(PassType, ControllerType)                            \
    UTL_STATIC_INIT {                                                          \
        registerConcretePass<PassType, ControllerType>(#PassType);             \
    }

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

REGISTER_PASS("Input", {
    .name = "Input",
    .outputs = {
        { .name = "Geometry" },
        { .name = "Lights" },
        { .name = "Bloom Parameters" },
        { .name = "Post Process Parameters" },
    },
    .color = InputNodeColor,
    .minSize = MinSize
});

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

REGISTER_PASS("Output", {
    .name = "Output",
    .inputs = { { .name = "Color" }, },
    .color = OutputNodeColor,
    .minSize = MinSize
});

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

AUTOREGISTER_PASS(ForwardShadingPass, ForwardShadingPassController);

struct PostProcessPassController: PassNodeController {
    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<PostProcessPass>("PostProcessPass"));
    }
};

AUTOREGISTER_PASS(PostProcessPass, PostProcessPassController);

struct BloomController: PassNodeController {
    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<BloomPass>("BloomPass"));
    }
};

AUTOREGISTER_PASS(BloomPass, BloomController);

} // namespace

RenderGraphNodeController* getController(Node const* node) {
    return dynamic_cast<RenderGraphNodeController*>(node->controller());
}

static std::string uniqueName(std::string_view name) {
    static int counter = 0;
    return utl::strcat(name, ++counter);
}

std::unique_ptr<RenderGraph> poppy::constructRenderGraph(
    RenderGraphContext& ctx, HardwareDevice& device,
    NodeEditorGraph const& model) {
    auto rg =
        std::make_unique<RenderGraph>(uniqueName("RenderGraph"), ctx, device);
    for (auto* controller: model.nodes() | transform(getController)) {
        controller->create(*rg);
    }
    for (auto* node: model.nodes()) {
        auto* sourceController = getController(node);
        BL_ASSERT(sourceController);
        for (auto [sourceIndex, output]: node->outputs() | enumerate) {
            auto* source = sourceController->getSource(sourceIndex);
            for (auto [sinkIndex, target]: output->targets() | enumerate) {
                auto* sinkController = getController(&target->parent());
                BL_ASSERT(sinkController);
                auto* sink = sinkController->getSink(sinkIndex);
                rg->addLink(source, sink);
            }
        }
    }
    if (!rg->compile()) {
        return nullptr;
    }
    return rg;
}

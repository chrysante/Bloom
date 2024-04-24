#include "Poppy/Views/RenderGraphNodeController.h"

#include <imgui.h>
#include <utl/common.hpp>
#include <utl/strcat.hpp>
#include <vml/vml.hpp>

#include "Bloom/Graphics/RenderGraph/BloomPass.h"
#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"
#include "Bloom/Graphics/RenderGraph/PostProcessPass.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/ShadowPass.h"
#include "Bloom/Shader/SharedTypes.h"

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

template <typename ControllerType>
static std::unique_ptr<RenderGraphNodeController> createController() {
    return std::make_unique<ControllerType>();
}

static PassRegistry& gRegistry() {
    static PassRegistry instance;
    return instance;
}

using ControllerFactory =
    std::function<std::unique_ptr<RenderGraphNodeController>()>;

static std::map<std::string, ControllerFactory>& factoryMap() {
    static std::map<std::string, ControllerFactory> map;
    return map;
}

std::unique_ptr<RenderGraphNodeController> RenderGraphNodeController::
    deserialize(YAML::Node node) {
    auto name = node["Name"].as<std::string>();
    if (auto factory = factoryMap()[name]) {
        return factory();
    }
    return nullptr;
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
            .controller = createController<ControllerType>(),
        };
    };
}

#define REGISTER_NODE(ControllerType, ...)                                     \
    UTL_STATIC_INIT {                                                          \
        auto& entry = registerPass();                                          \
        entry.name = ControllerType::StaticName();                             \
        entry.factory = [=] { return NodeDesc __VA_ARGS__; };                  \
        factoryMap()[ControllerType::StaticName()] =                           \
            createController<ControllerType>;                                  \
    }

#define REGISTER_PASS(PassType, ControllerType)                                \
    UTL_STATIC_INIT {                                                          \
        registerConcretePass<PassType, ControllerType>(#PassType);             \
        factoryMap()[#PassType] = createController<ControllerType>;            \
    }

namespace {

struct GlobalInputNodeController: RenderGraphNodeController {
    static std::string StaticName() { return "Input"; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Name"] = StaticName();
        return root;
    }

    void create(RenderGraph& rg) override {
        using enum InterfaceType;
        sources.clear();
        declare<Geometry>(rg, "Geometry", {});
        declare<Lights>(rg, "Lights", {});
        declare<Buffer>(rg, "Camera", { .size = sizeof(CameraInput) });
        declare<Buffer>(rg, "LightSpaceTransforms", { .size = 0 });
        declare<Buffer>(rg, "ShadowMetadata",
                        { .size = sizeof(ShadowRenderMetadata) });
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

REGISTER_NODE(GlobalInputNodeController, {
    .name = GlobalInputNodeController::StaticName(),
    .outputs = {
        { .name = "Geometry" },
        { .name = "Lights" },
        { .name = "Camera" },
        { .name = "Light Space Transforms" },
        { .name = "Shadow Metadata" },
        { .name = "Bloom Parameters" },
        { .name = "Post Process Parameters" },
    },
    .color = InputNodeColor,
    .minSize = MinSize,
    .controller = createController<GlobalInputNodeController>()
});

struct GlobalOutputNodeController: RenderGraphNodeController {
    static std::string StaticName() { return "Output"; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["Name"] = StaticName();
        return root;
    }

    void create(RenderGraph& rg) override {
        using enum InterfaceType;
        sinks.clear();
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

REGISTER_NODE(GlobalOutputNodeController, {
    .name = GlobalOutputNodeController::StaticName(),
    .inputs = { { .name = "Color" }, },
    .color = OutputNodeColor,
    .minSize = MinSize,
    .controller = createController<GlobalOutputNodeController>()
});

struct PassNodeController: RenderGraphNodeController {
    YAML::Node serialize() const override {
        YAML::Node root;
        root["Name"] = name;
        return root;
    }

    void contextMenuContent() override { ImGui::MenuItem("Preview"); }

    PassNodeController(std::string name): name(name) {}

    virtual Pass* createPass(RenderGraph& renderGraph) = 0;

    void create(RenderGraph& rg) final { pass = createPass(rg); }

    PassSource* getSource(size_t index) const final {
        return pass->source(index);
    }

    PassSink* getSink(size_t index) const final { return pass->sink(index); }

    Pass* pass;
    std::string name;
};

struct ForwardShadingPassController: PassNodeController {
    ForwardShadingPassController(): PassNodeController("ForwardShadingPass") {}

    Pass* createPass(RenderGraph& rg) override {
        pass = rg.addPass(std::make_unique<ForwardShadingPass>(name));
        return pass;
    }

    void bodyContent() override {
        if (!pass) {
            return;
        }
        auto* resource =
            dyncast<TextureResource const*>(pass->source(0)->resource());
        if (!resource) {
            return;
        }
        ImGui::Image(resource->handle().nativeHandle(),
                     ImGui::GetContentRegionAvail());
    }

    ForwardShadingPass* pass = nullptr;
};

REGISTER_PASS(ForwardShadingPass, ForwardShadingPassController);

struct ShadowPassController: PassNodeController {
    ShadowPassController(): PassNodeController("ShadowPass") {}

    Pass* createPass(RenderGraph& rg) override {
        pass = rg.addPass(std::make_unique<ShadowPass>(name));
        return pass;
    }

    void bodyContent() override {
        if (!pass) {
            return;
        }
        auto* resource =
            dyncast<TextureResource const*>(pass->source(0)->resource());
        if (!resource) {
            return;
        }
        ImGui::Image(resource->handle().nativeHandle(),
                     ImGui::GetContentRegionAvail());
    }

    ShadowPass* pass = nullptr;
    TextureHandle tex;
};

REGISTER_PASS(ShadowPass, ShadowPassController);

struct PostProcessPassController: PassNodeController {
    PostProcessPassController(): PassNodeController("PostProcessPass") {}

    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<PostProcessPass>(name));
    }
};

REGISTER_PASS(PostProcessPass, PostProcessPassController);

struct BloomController: PassNodeController {
    BloomController(): PassNodeController("BloomPass") {}

    Pass* createPass(RenderGraph& rg) override {
        return rg.addPass(std::make_unique<BloomPass>(name));
    }
};

REGISTER_PASS(BloomPass, BloomController);

} // namespace

RenderGraphNodeController* getController(Node const& node) {
    return dynamic_cast<RenderGraphNodeController*>(node.controller());
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
    for (auto* node: model.nodes()) {
        auto* controller = getController(*node);
        if (!controller) {
            Logger::Error("Ignoring node (no controller attached): ",
                          node->name());
            continue;
        }
        controller->create(*rg);
    }
    for (auto* node: model.nodes()) {
        auto* sourceController = getController(*node);
        if (!sourceController) {
            continue;
        }
        for (auto* output: node->outputs()) {
            auto* source = sourceController->getSource(output->indexInParent());
            for (auto* target: output->targets()) {
                auto* sinkController = getController(target->parent());
                if (!sinkController) {
                    continue;
                }
                auto* sink = sinkController->getSink(target->indexInParent());
                Logger::Trace("Linking ", source->qualifiedName(), " to ",
                              sink->qualifiedName());
                rg->addLink(source, sink);
            }
        }
    }
    if (!rg->compile()) {
        return nullptr;
    }
    return rg;
}

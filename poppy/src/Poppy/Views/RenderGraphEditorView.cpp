#include <functional>
#include <string>
#include <vector>

#include <imgui.h>
#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Graphics/RenderGraph/PassRegistry.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

enum CompilationStatus { None, Success, Error };

struct RenderGraphEditorView: View {
    struct Delegate: NodeEditorDelegate {
        Delegate(RenderGraphEditorView* view): view(view) {}

        void backgroundPopupMenu(float2 position) override {
            view->backgroundContextMenu(position);
        }

        RenderGraphEditorView* view;
    };

    RenderGraphEditorView();

    void init() override;

    void onInput(bloom::InputEvent& event) override;

    void display() override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    void compile();
    void doCompile();

    void backgroundContextMenu(float2 position);

    RenderGraph* renderGraph;
    NodeEditor nodeEditor;
    CompilationStatus compilationStatus = CompilationStatus::None;
};

POPPY_REGISTER_VIEW(RenderGraphEditorView, "Render Graph Editor");

static constexpr float4 InputNodeColor = { 0.4, 0.7, 0.2, 1.0 };
static constexpr float4 PassNodeColor = { 0.27, 0.58, 0.92, 1.0 };
static constexpr float4 OutputNodeColor = { 1.0, 0.0, 0.2, 1.0 };

class RenderGraphNodeController: public NodeController {
public:
    /// Creates this node in \p renderGraph
    virtual void create(bloom::rg::RenderGraph& renderGraph) = 0;

    /// \Returns the source at index \p index
    /// \Param index is the index in the node editor UI
    /// \Pre \p index must be valid and `create(RenderGraph)` must have been
    /// called before
    virtual bloom::rg::PassSource* getSource(size_t index) const = 0;

    /// \Returns the sink at index \p index
    /// \Param index is the index in the node editor UI
    /// \Pre \p index must be valid and `create(RenderGraph)` must have been
    /// called before
    virtual bloom::rg::PassSink* getSink(size_t index) const = 0;

    /// Counterpart to the non-static `NodeController::serialize`
    static std::unique_ptr<RenderGraphNodeController> deserialize(
        YAML::Node node);
};

struct GlobalInputNodeController: RenderGraphNodeController {
    static std::string StaticName() { return "Input"; }

    static NodeDesc MakeDesc(float2 position) {
        return { .name = StaticName(),
                 .position = position,
                 .controller = std::make_unique<GlobalInputNodeController>() };
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return {
            { .name = "Geometry" },
            { .name = "Lights" },
            { .name = "Camera" },
            { .name = "Light Space Transforms" },
            { .name = "Shadow Metadata" },
            { .name = "Bloom Parameters" },
            { .name = "Post Process Parameters" },
        };
    }

    float4 nodeColor() override { return InputNodeColor; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["ControllerName"] = StaticName();
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

    bool alwaysDisplayInputNames() const override { return false; }

    bool alwaysDisplayOutputNames() const override { return true; }

    template <InterfaceType Type>
    void declare(RenderGraph& rg, std::string name, InterfaceDesc<Type> desc) {
        RenderGraphSink* sink = rg.declareSink<Type>(std::move(name), desc);
        sources.push_back(sink->internalSource());
    }

    std::vector<PassSource*> sources;
};

struct GlobalOutputNodeController: RenderGraphNodeController {
    static std::string StaticName() { return "Output"; }

    static NodeDesc MakeDesc(float2 position) {
        return { .name = StaticName(),
                 .position = position,
                 .controller = std::make_unique<GlobalOutputNodeController>() };
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return { { .name = "Color" } };
    }

    float4 nodeColor() override { return OutputNodeColor; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["ControllerName"] = StaticName();
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

struct PassNodeController: RenderGraphNodeController {
    PassNodeController(PassRegistryEntry entry):
        registryEntry(std::move(entry)) {}

    static NodeDesc MakeDesc(float2 position, PassRegistryEntry entry) {
        // clang-format off
        return {
            .name = entry.name,
            .position = position,
            .controller = std::make_unique<PassNodeController>(entry),
        }; // clang-format on
    }

    std::vector<PinDesc> makeInputDescriptors() override {
        return registryEntry.vtable.sinks() | transform([](auto& interface) {
            return interfaceToPinDesc(interface);
        }) | ranges::to<std::vector>;
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return registryEntry.vtable.sources() | transform([](auto& interface) {
            return interfaceToPinDesc(interface);
        }) | ranges::to<std::vector>;
    }

    float4 nodeColor() override { return PassNodeColor; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["ControllerName"] = registryEntry.name;
        return root;
    }

    void contextMenuContent() override { ImGui::MenuItem("Preview"); }

    void create(RenderGraph& rg) final {
        pass = rg.addPass(registryEntry.vtable.make(registryEntry.name));
    }

    PassSource* getSource(size_t index) const final {
        return pass->source(index);
    }

    PassSink* getSink(size_t index) const final { return pass->sink(index); }

    Pass* pass;
    PassRegistryEntry registryEntry;
};

} // namespace

using ControllerFactory =
    std::function<std::unique_ptr<RenderGraphNodeController>()>;

static std::map<std::string, ControllerFactory>& factoryMap() {
    static auto map = [] {
        std::map<std::string, ControllerFactory> map;
        map[GlobalInputNodeController::StaticName()] = [] {
            return std::make_unique<GlobalInputNodeController>();
        };
        map[GlobalOutputNodeController::StaticName()] = [] {
            return std::make_unique<GlobalOutputNodeController>();
        };
        for (auto& entry: PassRegistry::entries()) {
            map[entry.name] = [=] {
                return std::make_unique<PassNodeController>(entry);
            };
        }
        return map;
    }();
    return map;
}

std::unique_ptr<RenderGraphNodeController> RenderGraphNodeController::
    deserialize(YAML::Node node) {
    auto name = node["ControllerName"].as<std::string>();
    auto& map = factoryMap();
    auto itr = map.find(name);
    if (itr != map.end()) {
        return itr->second();
    }
    Logger::Warn("Failed to deserialize node controller: name not found: ",
                 name);
    return nullptr;
}

static RenderGraphNodeController* getController(Node const& node) {
    return dynamic_cast<RenderGraphNodeController*>(node.controller());
}

static std::string uniqueName(std::string_view name) {
    static int counter = 0;
    return utl::strcat(name, ++counter);
}

static std::unique_ptr<RenderGraph> constructRenderGraph(
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

static char const* compStatusMessage(CompilationStatus status) {
    using enum CompilationStatus;
    switch (status) {
    case None:
        return "";
    case Success:
        return "Compile succeeded";
    case Error:
        return "Compile failed";
    }
}

RenderGraphEditorView::RenderGraphEditorView():
    nodeEditor(commandPaletteWrite()) {
    nodeEditor.setDelegate(std::make_unique<Delegate>(this));
    auto& cmd = commandPaletteWrite();
    cmd.add("Compile", { Key::B, ModFlags::Control }, [this] { compile(); });
    // clang-format off
    setToolbar({
        ToolbarIconButton(commandPalette().find("Compile"), "play"),
        ToolbarSpacer(),
        ToolbarTextItem()
            .text([this] { return compStatusMessage(compilationStatus); })
            .font(FontDesc::UIDefault().setSize(FontSize{ 16 }))
    }); // clang-format on
}

void RenderGraphEditorView::init() { setPadding({ 0, 0 }); }

void RenderGraphEditorView::onInput(InputEvent& event) {
    nodeEditor.onInput(event);
}

void RenderGraphEditorView::display() { nodeEditor.display(); }

YAML::Node RenderGraphEditorView::serialize() const {
    /// We use these functions to serialize our graph for now. This is not
    /// really correct, these functions should only serialize view related data,
    /// we will change this in the future
    return nodeEditor.model().serialize();
}

void RenderGraphEditorView::deserialize(YAML::Node root) {
    if (!root) {
        return;
    }
    NodeEditorGraph graph;
    graph.deserialize(root, RenderGraphNodeController::deserialize);
    nodeEditor.setGraph(std::move(graph));
}

void RenderGraphEditorView::compile() {
    dispatch(DispatchToken::NextFrame, [this] { doCompile(); });
}

void RenderGraphEditorView::doCompile() {
    auto& ctx = RenderGraphManager::context();
    ctx.resourcePool.clear();
    auto rg = constructRenderGraph(ctx, editor().device(), nodeEditor.model());
    if (rg) {
        compilationStatus = CompilationStatus::Success;
    }
    else {
        compilationStatus = CompilationStatus::Error;
    }
    RenderGraphManager::set(std::move(rg));
}

void RenderGraphEditorView::backgroundContextMenu(float2 position) {
    if (ImGui::MenuItem("Input")) {
        nodeEditor.addNode(GlobalInputNodeController::MakeDesc(position));
    }
    if (ImGui::MenuItem("Output")) {
        nodeEditor.addNode(GlobalOutputNodeController::MakeDesc(position));
    }
    for (auto& entry: PassRegistry::entries()) {
        if (ImGui::MenuItem(entry.name.c_str())) {
            nodeEditor.addNode(PassNodeController::MakeDesc(position, entry));
        }
    }
}

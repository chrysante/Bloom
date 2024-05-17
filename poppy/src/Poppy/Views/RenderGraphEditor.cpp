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
#include "Bloom/Graphics/Renderer.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Messages.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/NodeEditor.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

enum CompilationStatus { None, Success, Error };

struct RenderGraphEditor: View {
    struct Delegate: NodeEditorDelegate {
        Delegate(RenderGraphEditor* view): view(view) {}

        std::optional<float4> linkColor(PinUserType type) const override {
            switch ((InterfaceType)type) {
            case InterfaceType::BufferInterface:
                return float4(0, 0.5, 1, 1);
            case InterfaceType::TextureInterface:
                return float4(1, .2, 0, 1);
            case InterfaceType::GeometryInterface:
                return Appearance::get().style().colors.text;
            case InterfaceType::LightsInterface:
                return float4(0.2, 1, 0, 1);
            default:
                return std::nullopt;
            }
        }

        Node* createNodePopupMenu() override {
            return view->createNodePopupMenu();
        }

        std::unique_ptr<nodeEditor::NodeController> deserializeNodeController(
            YAML::Node const& node) override;

        RenderGraphEditor* view;
    };

    RenderGraphEditor();

    void init() override;

    void onInput(bloom::InputEvent& event) override;

    void display() override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    void compile();
    void doCompile();

    Node* createNodePopupMenu();

    RenderGraph* renderGraph;
    NodeEditorGraph model;
    NodeEditor nodeEditor;
    CompilationStatus compilationStatus = CompilationStatus::None;
};

POPPY_REGISTER_VIEW(RenderGraphEditor, "Render Graph Editor");

static constexpr float4 InputNodeColor = { 0.4, 0.7, 0.2, 1.0 };
static constexpr float4 PassNodeColor = { 0.27, 0.58, 0.92, 1.0 };
static constexpr float4 OutputNodeColor = { 1.0, 0.0, 0.2, 1.0 };

class RenderGraphNodeController: public NodeController {
public:
    /// Creates this node in \p renderGraph
    virtual void create(bloom::RenderGraph& renderGraph) = 0;

    /// \Returns the source at index \p index
    /// \Param index is the index in the node editor UI
    /// \Pre \p index must be valid and `create(RenderGraph)` must have been
    /// called before
    virtual bloom::PassSource* getSource(size_t index) const = 0;

    /// \Returns the sink at index \p index
    /// \Param index is the index in the node editor UI
    /// \Pre \p index must be valid and `create(RenderGraph)` must have been
    /// called before
    virtual bloom::PassSink* getSink(size_t index) const = 0;

    /// Counterpart to the non-static `NodeController::serialize`
    static std::unique_ptr<RenderGraphNodeController> deserialize(
        YAML::Node node);
};

std::unique_ptr<nodeEditor::NodeController> RenderGraphEditor::Delegate::
    deserializeNodeController(YAML::Node const& node) {
    return RenderGraphNodeController::deserialize(node);
}

static PinFlags translateFlags(InterfaceFlags flags) {
    PinFlags result{};
    if (test(flags & InterfaceFlags::Optional)) {
        result |= PinFlags::Optional;
    }
    return result;
}

static PinDesc interfaceToPinDesc(InterfaceUnion const& interface) {
    return PinDesc{ .name = interface.base().name(),
                    .flags = translateFlags(interface.base().flags()),
                    .userType = (PinUserType)interface.base().type() };
}

struct GlobalInputNodeController: RenderGraphNodeController {
    static std::string StaticName() { return "Input"; }

    static NodeDesc MakeDesc(float2 position) {
        return { .name = StaticName(),
                 .position = position,
                 .controller = std::make_unique<GlobalInputNodeController>() };
    }

    std::vector<PinDesc> makeOutputDescriptors() override {
        return Renderer::Sinks() | transform(interfaceToPinDesc) |
               ranges::to<std::vector>;
    }

    float4 nodeColor() override { return InputNodeColor; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["ControllerName"] = StaticName();
        return root;
    }

    void create(RenderGraph& rg) override {
        sources.clear();
        for (auto& interface: Renderer::Sinks()) {
            declareSink(rg, interface);
        }
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

    void declareSink(RenderGraph& rg, InterfaceUnion interface) {
        RenderGraphSink* sink = rg.declareSink(std::move(interface));
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
        return Renderer::Sources() | transform(interfaceToPinDesc) |
               ranges::to<std::vector>;
    }

    float4 nodeColor() override { return OutputNodeColor; }

    YAML::Node serialize() const override {
        YAML::Node root;
        root["ControllerName"] = StaticName();
        return root;
    }

    void create(RenderGraph& rg) override {
        sinks.clear();
        for (auto& interface: Renderer::Sources()) {
            declareSource(rg, interface);
        }
    }

    PassSource* getSource([[maybe_unused]] size_t index) const override {
        BL_UNREACHABLE();
    }

    PassSink* getSink(size_t index) const override {
        BL_ASSERT(index < sinks.size());
        return sinks[index];
    }

    void declareSource(RenderGraph& rg, InterfaceUnion interface) {
        RenderGraphSource* source = rg.declareSource(std::move(interface));
        sinks.push_back(source->internalSink());
    }

    std::vector<PassSink*> sinks;
};

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
    HardwareDevice& device, NodeEditorGraph const& model) {
    auto rg = std::make_unique<RenderGraph>(uniqueName("RenderGraph"), device);
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

RenderGraphEditor::RenderGraphEditor(): nodeEditor(commandPaletteWrite()) {
    nodeEditor.setDelegate(std::make_unique<Delegate>(this));
    nodeEditor.setModel(&model);
    auto& cmd = commandPaletteWrite();
    auto* compileCmd = cmd.add("Compile", { Key::B, ModFlags::Control },
                               [this] { compile(); });
    // clang-format off
    setToolbar({
        ToolbarIconButton(compileCmd, "ok"),
        ToolbarSpacer(),
        ToolbarTextItem()
            .text([this] { return compStatusMessage(compilationStatus); })
            .font(FontDesc::UIDefault().setSize(FontSize{ 16 }))
    }); // clang-format on
}

void RenderGraphEditor::init() {
    setPadding({ 0, 0 });
    listen([this](CompileRenderGraph) { compile(); });
}

void RenderGraphEditor::onInput(InputEvent& event) {
    nodeEditor.onInput(event);
}

void RenderGraphEditor::display() { nodeEditor.display(); }

YAML::Node RenderGraphEditor::serialize() const {
    YAML::Node root;
    root["NodeEditor"] = nodeEditor.serialize();
    /// We use these functions to serialize our graph for now. This is not
    /// really correct, these functions should only serialize view related data,
    /// we will change this in the future
    root["Model"] = model.serialize();
    return root;
}

void RenderGraphEditor::deserialize(YAML::Node root) {
    if (!root) {
        return;
    }
    nodeEditor.deserialize(root["NodeEditor"]);
    model.deserialize(root["Model"], RenderGraphNodeController::deserialize);
}

void RenderGraphEditor::compile() {
    dispatch(DispatchToken::NextFrame, [this] { doCompile(); });
}

void RenderGraphEditor::doCompile() {
    auto rg = constructRenderGraph(editor().device(), model);
    if (rg) {
        compilationStatus = CompilationStatus::Success;
    }
    else {
        compilationStatus = CompilationStatus::Error;
    }
    RenderGraphManager::set(std::move(rg));
}

Node* RenderGraphEditor::createNodePopupMenu() {
    Node* result = nullptr;
    if (ImGui::MenuItem("Input")) {
        result = model.addNode(GlobalInputNodeController::MakeDesc({}));
    }
    if (ImGui::MenuItem("Output")) {
        result = model.addNode(GlobalOutputNodeController::MakeDesc({}));
    }
    for (auto& entry: PassRegistry::entries()) {
        if (ImGui::MenuItem(entry.name.c_str())) {
            result = model.addNode(PassNodeController::MakeDesc({}, entry));
        }
    }
    return result;
}

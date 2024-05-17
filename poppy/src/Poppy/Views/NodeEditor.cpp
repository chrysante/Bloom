#include "Poppy/Views/NodeEditor.h"

#include <functional>
#include <optional>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/overload.hpp>
#include <utl/stack.hpp>
#include <utl/strcat.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/EnumCount.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Core/Yaml.h"
#include "Poppy/Core/SelectionManager.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Font.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

/// # Style

namespace {

struct NodeEditorStyleColors {
    float4 whiteOutline;
    float4 blackOutline;
    float4 nodeSelectedOutline;
    float4 nodeBody;
    float4 nodeBodyText;
    float4 nodeTitleText;
    float4 pinBG;
    float4 pinOptBG;
    float4 pinMissingIndicator;
    float4 pinLinkIndicator;
    float4 link;
    float4 backgroundGridline;
    float4 selectionRect;
    float4 selectionRectOutline;
};

struct NodeEditorStyleVars {
    float LinkWidth = 1.5;
    float LinkWidthHovered = 3;
    float LinkHoverDist = 7;
    /// The side length of the square that is occupied by a pin. This is the
    /// interaction area, the rendered size is smaller
    float PinAreaSize = 20;
    float PinRadius = 6;
    float PinConnectIndicatorRadius = 2;
    float PinConnectIndicatorRadiusHovered = 4;
    float NodeBodyTitleHeight = 20;
    /// Additional bottom padding because it looks better (or not)
    float NodeBodyBottomPadding = 0;
    float NodeBodyRounding = 10;
    float NodeBodySelectionOutlineInset = 2;
    /// How far the pins are moved into the node. If this is zero the outer edge
    /// of the node aligns with the edge of the pin
    float PinInset = 0;
    float2 MinNodeBodyPadding = 5;
    float2 BackgroundLineDist = { 30, 30 };
    float LinkBezierControlPointXMin = 100;
    float LinkBezierControlPointXMax = 100;
    float LinkBezierControlPointYFalloff = 150;
    float MinZoom = .25;
    float MaxZoom = 1;
};

struct NodeEditorStyle {
    EnumArray<AppearanceType, NodeEditorStyleColors> colors;
    NodeEditorStyleVars vars;

    NodeEditorStyle() {
        auto& light = colors[AppearanceType::Light];
        auto& dark = colors[AppearanceType::Dark];
        light.whiteOutline = { 1, 1, 1, 0.5 };
        light.blackOutline = { 0, 0, 0, 0.7 };
        light.nodeSelectedOutline = { 0, 0.6, 1, 1 };
        light.nodeBody = { 1, 1, 1, 1 };
        light.nodeBodyText = { 0, 0, 0, 1 };
        light.nodeTitleText = { 1, 1, 1, 1 };
        light.pinBG = { 0.2, 0.2, 0.2, 1 };
        light.pinOptBG = { 0.4, 0.4, 0.4, 1 };
        light.pinMissingIndicator = { 1, 0, 0.2, 1 };
        light.link = { 0, 0, 0, 0.5 };
        light.pinLinkIndicator = { 1, 1, 1, 0.8 };
        light.backgroundGridline = { 0, 0, 0, 0.2 };
        light.selectionRect = { 0, 0, 0, 0.14 };
        light.selectionRectOutline = { 0, 0, 0, 0.3 };

        dark.whiteOutline = light.whiteOutline;
        dark.blackOutline = light.blackOutline;
        dark.nodeSelectedOutline = light.nodeSelectedOutline;
        dark.nodeBody = { 0.08, 0.08, 0.08, 1 };
        dark.nodeBodyText = { 1, 1, 1, 1 };
        dark.nodeTitleText = light.nodeTitleText;
        dark.pinBG = { 1, 1, 1, 0.667 };
        dark.pinOptBG = { 1, 1, 1, 0.4 };
        dark.pinMissingIndicator = light.pinMissingIndicator;
        dark.link = { 1, 1, 1, 0.8 };
        dark.pinLinkIndicator = { 0, 0, 0, 0.8 };
        dark.backgroundGridline = { 0.33, 0.33, 0.33, 0.5 };
        dark.selectionRect = { 1, 1, 1, 0.3 };
        dark.selectionRectOutline = { 1, 1, 1, 0.3 };
    }
};

} // namespace

/// This is not const so we can edit the values with the style editor window
static NodeEditorStyle style;

static NodeEditorStyleColors const& styleColors() {
    return style.colors[Appearance::get().type()];
}

static NodeEditorStyleVars const& styleVars() { return style.vars; }

/// Derived properties

///
static float2 NodeBodyPadding() { return styleVars().MinNodeBodyPadding; }

static float2 NodeOuterPadding() {
    return vml::max(float2(1), float2(-styleVars().PinInset, 0));
}

static float PaddedTitleHeight() {
    return styleVars().NodeBodyTitleHeight + 2 * NodeBodyPadding().y;
}

static void styleEditor(bool* open) {
    auto& col = style.colors[Appearance::get().type()];
    auto& var = style.vars;
    ImGui::Begin("Node Editor Style Viewer", open);
    // clang-format off
    ImGui::SeparatorText("Style Colors");
    ImGui::ColorEdit4("White Outline", col.whiteOutline.data());
    ImGui::ColorEdit4("Black Outline", col.blackOutline.data());
    ImGui::ColorEdit4("Node Selection Outline", col.nodeSelectedOutline.data());
    ImGui::ColorEdit4("Node Body", col.nodeBody.data());
    ImGui::ColorEdit4("Node Body Text", col.nodeBodyText.data());
    ImGui::ColorEdit4("Node Title Text", col.nodeTitleText.data());
    ImGui::ColorEdit4("Link", col.link.data());
    ImGui::ColorEdit4("Background Grid Line", col.backgroundGridline.data());
    ImGui::ColorEdit4("Selection Rect", col.selectionRect.data());
    ImGui::ColorEdit4("Selection Rect Outline", col.selectionRectOutline.data());
    ImGui::ColorEdit4("Pin Background", col.pinBG.data());
    ImGui::ColorEdit4("Optional Pin Background", col.pinOptBG.data());
    ImGui::ColorEdit4("Missing Pin Indicator", col.pinMissingIndicator.data());
    ImGui::ColorEdit4("Pin Link Indicator", col.pinLinkIndicator.data());
    ImGui::SeparatorText("Style Vars");
    ImGui::SliderFloat("LinkWidth", &var.LinkWidth, 0, 10);
    ImGui::SliderFloat("LinkWidthHovered", &var.LinkWidthHovered, 0, 10);
    ImGui::SliderFloat("LinkHoverDist", &var.LinkHoverDist, 0, 20);
    ImGui::SliderFloat("PinAreaSize", &var.PinAreaSize, 0, 100);
    ImGui::SliderFloat("PinRadius", &var.PinRadius, 0, 100);
    ImGui::SliderFloat("PinConnectIndicatorRadius", &var.PinConnectIndicatorRadius, 0, var.PinRadius);
    ImGui::SliderFloat("PinConnectIndicatorRadiusHovered", &var.PinConnectIndicatorRadiusHovered, 0, var.PinRadius);
    ImGui::SliderFloat("NodeBodyTitleHeight", &var.NodeBodyTitleHeight, 0, 100);
    ImGui::SliderFloat("NodeBodyBottomPadding", &var.NodeBodyBottomPadding, 0, 100);
    ImGui::SliderFloat("NodeBodyRounding", &var.NodeBodyRounding, 0, 100);
    ImGui::SliderFloat("NodeBodySelectionOutlineInset", &var.NodeBodySelectionOutlineInset, 0, 100);
    ImGui::SliderFloat("PinInset", &var.PinInset, -100, 100);
    ImGui::SliderFloat2("MinNodeBodyPadding", var.MinNodeBodyPadding.data(), 0, 50);
    ImGui::SliderFloat2("BackgroundLineDist", var.BackgroundLineDist.data(), 0, 200);
    ImGui::SliderFloat("LinkBezierControlPointXMin", &var.LinkBezierControlPointXMin, 0, var.LinkBezierControlPointXMax);
    ImGui::SliderFloat("LinkBezierControlPointXMax", &var.LinkBezierControlPointXMax, var.LinkBezierControlPointXMin, 1000);
    ImGui::SliderFloat("LinkBezierControlPointYFalloff", &var.LinkBezierControlPointYFalloff, 0, 1000);
    ImGui::SliderFloat("MinZoom", &var.MinZoom, 0.01, var.MaxZoom);
    ImGui::SliderFloat("MaxZoom", &var.MaxZoom, var.MinZoom, 1000);
    // clang-format on
    ImGui::End();
}

using Impl = NodeEditor::Impl;

static constexpr auto LinkPopupName = "LinkPopup";
static constexpr auto BackgroundPopupName = "BackgroundPopup";
static constexpr auto DraggingLinkReleasedPopupName =
    "DraggingLinkReleasedPopup";

static ImRect toRect(float2 begin, float2 end, auto... rest) {
    return ImRect(vml::min(begin, end, rest...), vml::max(begin, end, rest...));
}

static ImRect toRect(Node const& node) {
    return ImRect(node.position(), node.position() + node.size());
}

size_t Pin::indexInParent() const {
    auto& node = parent();
    // clang-format off
    return visit(*this, utl::overload{
        [&](OutputPin const& pin) {
            auto itr = ranges::find(node._outputs, &pin, 
                                    [](auto& p) { return p.get(); });
            BL_ASSERT(itr != node._outputs.end());
            return utl::narrow_cast<size_t>(itr - node._outputs.begin());
        },
        [&](InputPin const& pin) {
            auto itr = ranges::find(node._inputs, &pin, 
                                    [](auto& p) { return p.get(); });
            BL_ASSERT(itr != node._inputs.end());
            return utl::narrow_cast<size_t>(itr - node._inputs.begin());
        }
    }); // clang-format on
}

static void clearLinksImpl(Pin& pin, auto callback) {
    // clang-format off
    visit(pin, utl::overload{
        [&](OutputPin& pin) {
            auto targets = pin.targets() | ToSmallVector<>;
            pin.clearTargets();
            callback(pin, targets);
        },
        [&](InputPin& pin) {
            auto* origin = pin.origin();
            pin.setOrigin(nullptr);
            callback(pin, origin);
        }
    }); // clang-format on
}

static void clearLinksWithCallback(Pin& pin) {
    // clang-format off
    clearLinksImpl(pin, utl::overload{
        [](OutputPin& pin, std::span<InputPin* const> targets) {
            for (auto* target: targets) {
                target->parent().controller()->pinConnectionChanged(*target);
            }
            pin.parent().controller()->pinConnectionChanged(pin);
        },
        [](InputPin& pin, OutputPin* origin) {
            if (origin) {
                origin->parent().controller()->pinConnectionChanged(*origin);
            }
            pin.parent().controller()->pinConnectionChanged(pin);
        }
    }); // clang-format on
}

void Pin::clearLinks() {
    clearLinksImpl(*this, [](auto&&...) {});
}

void InputPin::setOrigin(OutputPin* origin) {
    if (_origin == origin) {
        return;
    }
    auto* last = _origin;
    _origin = origin;
    if (last) {
        last->removeTargetWeak(this);
    }
}

void OutputPin::addTarget(InputPin* target) {
    if (!isTarget(target)) {
        _targets.push_back(target);
    }
}

bool OutputPin::isTarget(InputPin const* target) const {
    return ranges::contains(_targets, target);
}

void OutputPin::removeTarget(InputPin* pin) {
    if (removeTargetWeak(pin)) {
        pin->setOrigin(nullptr);
    }
}

bool OutputPin::removeTargetWeak(InputPin* pin) {
    auto itr = ranges::find(_targets, pin);
    if (itr != _targets.end()) {
        _targets.erase(itr);
        return true;
    }
    return false;
}

void OutputPin::clearTargets() {
    for (auto* target: targets()) {
        target->setOrigin(nullptr);
    }
    _targets.clear();
}

static bool findCycleDFS(Node const* node, utl::stack<Node const*>& stack) {
    return ranges::any_of(node->successors(), [&](Node const* succ) {
        if (ranges::contains(stack, succ)) {
            return true;
        }
        stack.push(succ);
        bool result = findCycleDFS(succ, stack);
        stack.pop();
        return result;
    });
}

static bool findCycle(Node const* node) {
    utl::stack<Node const*> stack;
    return findCycleDFS(node, stack);
}

void nodeEditor::link(OutputPin& out, InputPin& in) {
    BL_ASSERT(!findCycle(&out.parent()));
    out._targets.push_back(&in);
    bool wouldIntroduceCycle = findCycle(&out.parent());
    out._targets.pop_back();
    if (wouldIntroduceCycle) {
        Logger::Error("Link would introduce a cycle. Not linking");
        return;
    }
    out.addTarget(&in);
    in.setOrigin(&out);
    in.parent().controller()->pinConnectionChanged(in);
    out.parent().controller()->pinConnectionChanged(out);
}

/// Links the pins \p a and \p b if compatible
static void link(Pin& a, Pin& b) {
    // clang-format off
    visit(a, b, utl::overload{
        [&](InputPin& in, OutputPin& out) {
            link(out, in);
        },
        [&](OutputPin& out, InputPin& in) {
            link(out, in);
        },
        [&](Pin&, Pin&) {},
    }); // clang-format on
}

/// \Returns `true` if there is a link between pin \p a and pin \p b
static bool isLinked(Pin const& a, Pin const& b) {
    // clang-format off
    return visit(a, b, utl::overload{
        [&](InputPin const& in, OutputPin const& out) {
            return in.origin() == &out;
        },
        [&](OutputPin const& out, InputPin const& in) {
            return in.origin() == &out;
        },
        [&](Pin const&, Pin const&) { return false; },
    }); // clang-format on
}

static auto succsImpl(auto const& outputs) {
    return outputs |
           ranges::views::transform([](auto* pin) { return pin->targets(); }) |
           ranges::views::join |
           ranges::views::transform([](auto* pin) { return &pin->parent(); });
}

Node::~Node() {
    for (auto* pin: inputs()) {
        pin->clearLinks();
    }
    for (auto* pin: outputs()) {
        pin->clearLinks();
    }
}

utl::hashset<Node*> Node::successors() {
    return succsImpl(outputs()) | ranges::to<utl::hashset<Node*>>;
}

utl::hashset<Node const*> Node::successors() const {
    return succsImpl(outputs()) | ranges::to<utl::hashset<Node const*>>;
}

static auto predsImpl(auto const& inputs) {
    return inputs |
           ranges::views::transform([](auto* pin) { return pin->origin(); }) |
           ranges::views::filter([](auto* node) { return node != nullptr; }) |
           ranges::views::transform([](auto* pin) { return &pin->parent(); });
}

utl::hashset<Node*> Node::predecessors() {
    return predsImpl(inputs()) | ranges::to<utl::hashset<Node*>>;
}

utl::hashset<Node const*> Node::predecessors() const {
    return predsImpl(inputs()) | ranges::to<utl::hashset<Node const*>>;
}

Node* NodeEditorGraph::addNode(std::unique_ptr<Node> node) {
    BL_ASSERT(node, "Node must not be null");
    _nodes.push_back(std::move(node));
    return _nodes.back().get();
}

Node* NodeEditorGraph::addNode(NodeDesc desc) {
    return addNode(std::make_unique<Node>(std::move(desc)));
}

bool NodeEditorGraph::hasCycles() const {
    return ranges::any_of(sources(),
                          [](auto* source) { return findCycle(source); });
}

static auto sourcesImpl(auto const& nodes) {
    return nodes | filter([](Node const* node) {
        return ranges::all_of(node->inputs(), [](InputPin const* pin) {
            return pin->origin() == nullptr;
        });
    });
}

utl::small_vector<Node*> NodeEditorGraph::sources() {
    return sourcesImpl(nodes()) | ranges::to<utl::small_vector<Node*>>;
}

utl::small_vector<Node const*> NodeEditorGraph::sources() const {
    return sourcesImpl(nodes()) | ranges::to<utl::small_vector<Node const*>>;
}

static auto sinksImpl(auto const& nodes) {
    return nodes | filter([](Node const* node) {
        return ranges::all_of(node->outputs(), [](OutputPin const* pin) {
            return pin->targets().empty();
        });
    });
}

utl::small_vector<Node*> NodeEditorGraph::sinks() {
    return sinksImpl(nodes()) | ranges::to<utl::small_vector<Node*>>;
}

utl::small_vector<Node const*> NodeEditorGraph::sinks() const {
    return sinksImpl(nodes()) | ranges::to<utl::small_vector<Node const*>>;
}

static YAML::Node serializeNode(Node const& node, utl::uuid ID) {
    auto& desc = node.desc();
    YAML::Node root;
    root["ID"] = ID;
    root["Name"] = desc.name;
    root["MinSize"] = desc.minSize;
    root["Position"] = desc.position;
    root["Resizable"] = desc.resizable;
    if (desc.resizable) {
        root["Size"] = node.size();
    }
    root["Controller"] = desc.controller->serialize();
    return root;
}

static YAML::Node serializeLink(OutputPin const& out, utl::uuid outID,
                                InputPin const& in, utl::uuid inID) {
    YAML::Node root;
    root["FromID"] = outID;
    root["FromIndex"] = out.indexInParent();
    root["ToID"] = inID;
    root["ToIndex"] = in.indexInParent();
    return root;
}

namespace {

struct SerializationOptions {
    /// If set to true, the existing node IDs are written, otherwise new IDs are
    /// generated
    bool generateNewIDs = false;
    /// If set to false, `serializeNodes()` will only serialize links to nodes
    /// given to the function, i.e., if only a subset of nodes is serialized,
    /// only links to nodes within the subset are written
    bool serializeLinksToOtherNodes = true;
};

} // namespace

static YAML::Node serializeNodes(std::span<Node const* const> nodes,
                                 SerializationOptions options = {}) {
    YAML::Node root;
    auto nodeList = root["Nodes"];
    auto linkList = root["Links"];
    utl::hashmap<Node const*, utl::uuid> nodeIDMap;
    for (auto* node: nodes) {
        if (!node->controller()) {
            Logger::Error("Failed to serialize node without controller: ",
                          node->name());
            continue;
        }
        auto ID = options.generateNewIDs ? utl::uuid::generate() : node->ID();
        nodeIDMap.insert({ node, ID });
        nodeList.push_back(serializeNode(*node, ID));
    }
    utl::hashset<Node const*> nodeSet;
    if (!options.serializeLinksToOtherNodes) {
        nodeSet = nodes | ranges::to<utl::hashset<Node const*>>;
    }
    for (auto* node: nodes) {
        for (auto* input: node->inputs()) {
            if (!input->origin()) {
                continue;
            }
            if (!options.serializeLinksToOtherNodes &&
                !nodeSet.contains(&input->origin()->parent()))
            {
                continue;
            }
            auto* origin = input->origin();
            linkList.push_back(
                serializeLink(*origin, nodeIDMap[&origin->parent()], *input,
                              nodeIDMap[&input->parent()]));
        }
    }
    return root;
}

YAML::Node NodeEditorGraph::serialize() const {
    return serializeNodes(nodes() | ToSmallVector<>);
}

using ControllerFactory =
    utl::function_view<std::unique_ptr<NodeController>(YAML::Node const&)>;

static std::unique_ptr<Node> deserializeNode(
    YAML::Node const& source, ControllerFactory deserializeController) {
    NodeDesc desc{};
    desc.ID = source["ID"].as<utl::uuid>();
    desc.name = source["Name"].as<std::string>();
    desc.minSize = source["MinSize"].as<vml::float2>();
    desc.position = source["Position"].as<vml::float2>();
    bool resizable = desc.resizable = source["Resizable"].as<bool>();
    desc.controller = deserializeController(source["Controller"]);
    auto node = std::make_unique<Node>(std::move(desc));
    if (resizable) {
        node->setSize(source["Size"].as<vml::float2>());
    }
    return node;
}

static Node* findNode(utl::hashmap<utl::uuid, Node*> const& map, utl::uuid ID) {
    auto itr = map.find(ID);
    if (itr != map.end()) {
        return itr->second;
    }
    return nullptr;
}

static void deserializeLink(YAML::Node const& source,
                            utl::hashmap<utl::uuid, Node*> const& map) {

    auto fromID = source["FromID"].as<utl::uuid>();
    auto fromIndex = source["FromIndex"].as<size_t>();
    auto toID = source["ToID"].as<utl::uuid>();
    auto toIndex = source["ToIndex"].as<size_t>();
    Node* fromNode = findNode(map, fromID);
    Node* toNode = findNode(map, toID);
    if (!fromNode || !toNode) {
        Logger::Warn("Failed to deserialize link: node not found");
        return;
    }
    if (fromIndex >= fromNode->outputs().size()) {
        Logger::Warn("Failed to deserialize link: output index out of range: ",
                     fromIndex, " (on node ", fromNode->name(), ")");
        return;
    }
    if (toIndex >= toNode->inputs().size()) {
        Logger::Warn("Failed to deserialize link: input index out of range: ",
                     toIndex, " (on node ", toNode->name(), ")");
        return;
    }
    link(*fromNode->output(fromIndex), *toNode->input(toIndex));
}

static YAML::Node getValue(YAML::Node const& root, std::string_view key) {
    try {
        return root[key];
    }
    catch (YAML::Exception const&) {
        return {};
    }
}

static utl::small_vector<Node*> deserializeNodes(
    NodeEditorGraph& dest, YAML::Node source,
    ControllerFactory deserializeController,
    utl::function_view<void(Node&)> transform = {}) {
    utl::small_vector<Node*> allNodes;
    utl::hashmap<utl::uuid, Node*> map;
    auto nodeList = getValue(source, "Nodes");
    if (!nodeList.IsDefined()) {
        Logger::Error("No nodes in yaml node");
        return {};
    }
    for (auto nodeSource: nodeList) {
        std::unique_ptr<Node> node;
        try {
            node = deserializeNode(nodeSource, deserializeController);
        }
        catch (YAML::Exception const& e) {
            Logger::Error("Failed to deserialize node: ", e.what());
        }
        if (!node) {
            continue;
        }
        if (transform) {
            transform(*node);
        }
        if (!utl::is_null(node->ID())) {
            map[node->ID()] = node.get();
        }
        allNodes.push_back(node.get());
        dest.addNode(std::move(node));
    }
    auto linkList = getValue(source, "Links");
    if (!linkList.IsDefined()) {
        Logger::Error("No links in yaml node");
        return {};
    }
    for (auto linkSource: linkList) {
        try {
            deserializeLink(linkSource, map);
        }
        catch (YAML::Exception const& e) {
            Logger::Error("Failed to deserialize: ", e.what());
        }
    }
    return allNodes;
}

void NodeEditorGraph::deserialize(YAML::Node const& source,
                                  ControllerFactory deserializeController) {
    _nodes.clear();
    deserializeNodes(*this, source, deserializeController);
}

void NodeEditorGraph::deleteIf(utl::function_view<bool(Node const&)> pred) {
    auto itr =
        ranges::remove_if(_nodes, [&](auto& node) { return pred(*node); });
    _nodes.erase(itr, _nodes.end());
}

void NodeEditorGraph::orderFront(Node* node) {
    auto itr =
        ranges::find_if(_nodes, [&](auto& n) { return n.get() == node; });
    BL_ASSERT(itr != _nodes.end(), "Node not found");
    auto n = std::move(*itr);
    _nodes.erase(itr);
    _nodes.push_back(std::move(n));
}

namespace {

struct Link {
    OutputPin* origin;
    InputPin* dest;

    bool operator==(Link const&) const = default;

    bool contains(Pin const* pin) const {
        if (!pin) {
            return false;
        }
        // clang-format off
        return visit(*pin, utl::overload {
            [&](OutputPin const& pin) { return &pin == origin; },
            [&](InputPin const& pin) { return &pin == dest; }
        }); // clang-format on
    }

    explicit operator bool() const { return origin && dest; }
};

static void deleteLink(Link link) { clearLinksWithCallback(*link.dest); }

enum class Axis { Horizontal, Vertical };

struct PersistentViewData {
    /// Position of the view in surface coordinates
    vml::float2 position = { 0.0, 0.0 };
    float zoom = 1.0;
    /// In screen coordinates
    float2 popupPosition = {};
    /// This is here and not in `ActiveViewData` because we want to prolong its
    /// lifetime when the dragging link popup menu is open
    Pin* activePin = nullptr;
    char const* activePopupName = nullptr;
};

} // namespace

BLOOM_MAKE_TEXT_SERIALIZER(PersistentViewData, position, zoom);

namespace {

/// Data that persists between frames when performing some actions.
/// This is reset whenever a mouse button is released
struct ActiveViewData {
    /// True if we already changed selection state since the mouse was pressed
    bool handledSelectionSinceMousePressed = false;
    /// True if we are selecting be dragged the background
    bool isSelectionDragging = false;
    bool beginMoveDragging = false;
    bool isMoveDragging = false;
    float2 resizeSize = 0;
    /// In screen coordinates
    float2 selectionBegin = 0;
    float2 selectionEnd = 0;
    /// Set to the node currently being dragged
    Node* draggingNode = nullptr;
    Node* activeNode = nullptr;
    Node* activeResize = nullptr;

    ImRect getSelectionRect() const {
        return toRect(selectionBegin, selectionEnd);
    }
};

template <typename T>
struct HoverItem {
    void set(T item) { thisFrame = item; }

    T get() const { return thisFrame == lastFrame ? thisFrame : T(); }

    T getLast() const { return lastFrame; }

    void update() {
        lastFrame = thisFrame;
        thisFrame = T();
    }

private:
    T thisFrame{}, lastFrame{};
};

/// This is reset at the start of each new frame
struct FrameViewData {
    ImRect clipRect = {};
    vml::rectangle<float> nodeRectUnion = {};
    bool windowHovered = false;
    bool windowOrChildHovered = false;
    bool disableBackgroundScrolling = false;
    HoverItem<Pin*> hoveredPin;
    HoverItem<Node*> hoveredNode;
    HoverItem<Link> hoveredLink;
    HoverItem<Link> hoveredLinkUnderNode;

    void update() {
        auto* window = ImGui::GetCurrentWindow();
        clipRect = window->ContentRegionRect;
        hoveredPin.update();
        hoveredNode.update();
        hoveredLink.update();
        hoveredLinkUnderNode.update();
        windowHovered = false;
        windowOrChildHovered = false;
        disableBackgroundScrolling = false;
    }
};

struct ViewData: PersistentViewData, ActiveViewData, FrameViewData {
    Link popupLink = {};

    void beginFrame() { FrameViewData::update(); }

    void resetActiveData() {
        static_cast<ActiveViewData&>(*this) = ActiveViewData{};
    }
};

static bool drawDebugHelpers = false;
static bool displayPinUserType = false;

static void debuggerItem(char const* name, Node const* node) {
    ImGui::Text("%s: %s", name, node ? node->name().data() : "");
}

static void debuggerItem(char const* name, Pin const* pin) {
    ImGui::Text("%s: %s", name, pin ? pin->name().data() : "");
}

static void debuggerItem(char const* name, Link link) {
    if (!link) {
        ImGui::Text("%s", name);
    }
    else {
        ImGui::Text("%s: %s -> %s", name, link.origin->name().data(),
                    link.dest->name().data());
    }
}

static void debuggerItem(char const* name, float value) {
    ImGui::Text("%s: %f", name, value);
}

static void debuggerItem(char const* name, float2 value) {
    ImGui::Text("%s: (%f, %f)", name, value.x, value.y);
}

static void debuggerItem(char const* name, bool value) {
    ImGui::Text("%s: %s", name, value ? "true" : "false");
}

static void debuggerItem(char const* name, char const* value) {
    ImGui::Text("%s: %s", name, value ? value : "NULL");
}

template <typename T>
static void debuggerItem(char const* name, HoverItem<T> item) {
    return debuggerItem(name, item.get());
}

static void displayDebugger(ViewData const& viewData,
                            std::span<Node const* const> selection,
                            bool* open) {
#define ITEM(name) debuggerItem(#name, viewData.name)
    ImGui::Begin("Node Editor Debugger", open);
    ImGui::Checkbox("Draw Debug Helpers", &drawDebugHelpers);
    ImGui::Checkbox("Display Pin User Types", &displayPinUserType);
    ImGui::SeparatorText("Persistent Data");
    ITEM(position);
    ITEM(zoom);
    ImGui::SeparatorText("Active Data");
    ITEM(draggingNode);
    ITEM(handledSelectionSinceMousePressed);
    ITEM(isSelectionDragging);
    ITEM(beginMoveDragging);
    ITEM(isMoveDragging);
    ITEM(resizeSize);
    ITEM(selectionBegin);
    ITEM(selectionEnd);
    ITEM(activeNode);
    ITEM(activeResize);
    ITEM(activePin);
    ITEM(activePopupName);
    ITEM(popupPosition);
    ITEM(nodeRectUnion.lower_bound());
    ITEM(nodeRectUnion.upper_bound());
    ImGui::SeparatorText("Frame Data");
    ITEM(windowHovered);
    ITEM(windowOrChildHovered);
    ITEM(hoveredPin);
    ITEM(hoveredNode);
    ITEM(hoveredLink);
    ITEM(hoveredLinkUnderNode);
    ImGui::SeparatorText("Selection");
    for (auto* node: selection) {
        ImGui::Text("%s",
                    utl::strcat(node->name(), " ", node->position()).c_str());
    }
    ImGui::End();
#undef ITEM
}

using SelectionManager = BasicSelectionManager<Node*>;

} // namespace

struct Node::LayoutData {
    /// Minimal node size due to layout constrains
    float2 minSize = 0;
};

Node::Node(NodeDesc desc):
    _desc(std::move(desc)), _layoutData(std::make_unique<LayoutData>()) {
    if (utl::is_null(_desc.ID)) {
        _desc.ID = utl::uuid::generate();
    }
    if (controller()) {
        controller()->_node = this;
        _color = controller()->nodeColor();
        auto inputDescs = controller()->makeInputDescriptors();
        _inputs = inputDescs | transform([&](PinDesc const& pin) {
            return DynUniquePtr<InputPin>(new InputPin(pin, *this));
        }) | ranges::to<std::vector>;
        auto outputDescs = controller()->makeOutputDescriptors();
        _outputs = outputDescs | transform([&](PinDesc const& pin) {
            return DynUniquePtr<OutputPin>(new OutputPin(pin, *this));
        }) | ranges::to<std::vector>;
    }
}

struct NodeEditor::Impl {
    explicit Impl(CommandPalette& commandPalette);

    void onInput(bloom::InputEvent& event);

    void display();

    bool isUserTypeCompatible(Pin const& a, Pin const& b) const;
    bool isLinkable(Pin const& a, Pin const& b) const;
    float4 getLinkColor(PinUserType type) const;
    void linkPopup();
    void draggingLinkReleasedPopup();
    void backgroundPopup();
    void displayPopups();
    void backgroundBehaviour();
    void drawOverlays();
    void displayLinks(Node& node);
    void displayPinLink(InputPin& pin, float2 pinPosition);
    void displayDraggingLink();
    void pinPopup(Pin& pin, bool show);
    void displayPin(Pin& pin, float2 position, float& maxLabelWith,
                    bool displayLabelInBody);
    void displayPins(Node& node);
    void nodeBodyPopup(Node& node, bool show);
    void displayNodeBody(Node& node);
    bool canDropNodeIntoLink(Node const& node, Link link) const;
    void dropNodeIntoLink(Node& node, Link link);
    void displayNode(Node& node);
    void handleDraggingLinkMouseReleased();

    void orderFront(Node* node);

    void deleteSelected();
    void copyNodesToClipboard(std::span<Node const* const> nodes);
    void pasteNodesFromClipboard();

    void openPopup(char const* name);
    void defer(auto f) { deferList.push_back(f); }

    NodeEditorGraph* model = nullptr;
    SelectionManager selection;
    ViewData viewData;
    std::unique_ptr<NodeEditorDelegate> delegate;
    std::vector<std::function<void()>> deferList;
    bool showDebugger = false;
    bool showStyleEditor = false;
};

NodeEditor::NodeEditor(CommandPalette& commandPalette):
    impl(std::make_unique<Impl>(commandPalette)) {}

NodeEditor::~NodeEditor() = default;

void NodeEditor::setDelegate(std::unique_ptr<NodeEditorDelegate> delegate) {
    impl->delegate = std::move(delegate);
}

NodeEditorDelegate* NodeEditor::getDelegate() { return impl->delegate.get(); }

static bool beginInvisibleWindow(
    char const* name, float2 pos, float2 size,
    ImGuiWindowFlags flags = ImGuiWindowFlags_None) {
    ImGui::SetNextWindowPos(pos);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32_BLACK_TRANS);
    flags |= ImGuiWindowFlags_NoDecoration;
    bool open = ImGui::BeginChildEx(name, ImGui::GetID(name), size,
                                    ImGuiChildFlags_None, flags);
    ImGui::PopStyleColor();
    return open;
}

/// Draws lines perpendicular to \p axis starting from coordinate \p begin
/// spanning the entire width or height of the current view
static void drawLines(float begin, float end, float dist, Axis axis,
                      float4 color) {
    auto* DL = ImGui::GetWindowDrawList();
    float2 p1 = { begin, 0.0 }, p2 = { begin, 0.0 };
    float* pos1 = nullptr;
    float* pos2 = nullptr;
    float2 winpos = ImGui::GetWindowPos();
    switch (axis) {
    case Axis::Horizontal:
        end += winpos.x;
        p1.x += winpos.x;
        p2.x += winpos.x;
        p1.y = winpos.y;
        p2.y = winpos.y + ImGui::GetWindowHeight();
        pos1 = &p1.x;
        pos2 = &p2.x;
        break;
    case Axis::Vertical:
        end += winpos.y;
        p1 = p1.swizzle(1, 0);
        p2 = p2.swizzle(1, 0);
        p1.y += winpos.y;
        p2.y += winpos.y;
        p1.x = winpos.x;
        p2.x = winpos.x + ImGui::GetWindowWidth();
        pos1 = &p1.y;
        pos2 = &p2.y;
        break;
    }
    auto colU32 = ImGui::ColorConvertFloat4ToU32(color);
    for (; *pos1 <= end; *pos1 += dist, *pos2 += dist) {
        DL->AddLine(p1, p2, colU32);
    }
}

/// \p center is in screen coordinates
[[maybe_unused]] static void drawCross(float2 center, float size, ImU32 col) {
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddLine(center - float2(size, 0), center + float2(size, 0), col);
    DL->AddLine(center - float2(0, size), center + float2(0, size), col);
}

static void drawBackground(ViewData const& viewData, float zoom) {
    while (zoom <= 0.5) {
        zoom *= 2;
    }
    while (zoom >= 2) {
        zoom /= 2;
    }
    float4 color = styleColors().backgroundGridline;
    float2 dist = styleVars().BackgroundLineDist * zoom;
    float2 begin = vml::fmod(-viewData.position, dist);
    drawLines(begin.x, ImGui::GetWindowWidth(), dist.x, Axis::Horizontal,
              color);
    drawLines(begin.y, ImGui::GetWindowHeight(), dist.y, Axis::Vertical, color);
    if (drawDebugHelpers) {
        float2 viewPosSS = (float2)ImGui::GetWindowPos() - viewData.position;
        drawCross(viewPosSS, 250, IM_COL32_WHITE);
        auto* DL = ImGui::GetWindowDrawList();
        DL->AddRect(viewPosSS +
                        viewData.nodeRectUnion.lower_bound() * viewData.zoom,
                    viewPosSS +
                        viewData.nodeRectUnion.upper_bound() * viewData.zoom,
                    IM_COL32(255, 0, 255, 255));
    }
}

static bool nodeOverlapsRect(ViewData const& viewData, Node const& node,
                             ImRect rect) {
    auto nodeRect = toRect(node);
    nodeRect.Min = (float2)nodeRect.Min * viewData.zoom;
    nodeRect.Max = (float2)nodeRect.Max * viewData.zoom;
    nodeRect.Translate((float2)viewData.clipRect.Min - viewData.position);
    return rect.Overlaps(nodeRect);
}

bool Impl::isUserTypeCompatible(Pin const& a, Pin const& b) const {
    auto impl = [&](OutputPin const& a, InputPin const& b) {
        if (!delegate) {
            return a.userType() == b.userType();
        }
        return delegate->isPinTypeCompatible(a.userType(), b.userType());
    };
    // clang-format off
    return visit(a, b, utl::overload{
        impl,
        [&](InputPin const& b, OutputPin const& a) {
            return impl(a, b);
        },
        [](Pin const&, Pin const&) { return false; }
    }); // clang-format on
}

static bool isIOCompatible(Pin const& a, Pin const& b) {
    return &a.parent() != &b.parent() && a.type() != b.type();
}

bool Impl::isLinkable(Pin const& a, Pin const& b) const {
    return isIOCompatible(a, b) && isUserTypeCompatible(a, b);
}

/// Computes the position of \p pin relative to its parent node
/// This is the single source of truth of pin positions
static float2 computePinPosition(Pin const& pin, size_t index) {
    float posy = PaddedTitleHeight() + index * styleVars().PinAreaSize;
    // clang-format off
    return visit(pin, utl::overload{
        [&](InputPin const&) {
            return float2(styleVars().PinInset, posy);
        },
        [&](OutputPin const& pin) {
            return float2(pin.parent().size().x - styleVars().PinInset -
                              styleVars().PinAreaSize,
                          posy);
        }
    }); // clang-format on
}

/// Computes the position of \p pin relative to the node "world" / surface
static float2 computeWorldPinPosition(Pin const& pin) {
    return pin.parent().position() +
           computePinPosition(pin, pin.indexInParent());
}

/// Computes the position of \p pin in screen coordinates
static float2 computeScreenPinPosition(ViewData const& viewData,
                                       Pin const& pin) {
    float2 basePos = (float2)viewData.clipRect.Min - viewData.position;
    return basePos + computeWorldPinPosition(pin) * viewData.zoom;
}

float4 Impl::getLinkColor(PinUserType type) const {
    if (!delegate) {
        return styleColors().link;
    }
    return delegate->linkColor(type).value_or(styleColors().link);
}

static bool beginPopup(ViewData& viewData, char const* name,
                       auto closeCallback) {
    bool result = ImGui::BeginPopup(name);
    if (result) {
        viewData.activePopupName = name;
    }
    else if ((void*)viewData.activePopupName == (void*)name) {
        viewData.activePopupName = nullptr;
        closeCallback();
    }
    return result;
}

static bool beginPopup(ViewData& viewData, char const* name) {
    return beginPopup(viewData, name, []() {});
}

void Impl::linkPopup() {
    if (!beginPopup(viewData, LinkPopupName)) {
        return;
    }
    if (ImGui::MenuItem("Delete Link")) {
        deleteLink(viewData.popupLink);
    }
    ImGui::EndPopup();
}

static float2 screenToWorldSpace(float2 position, ViewData const& viewData) {
    float2 posVS = position - (float2)viewData.clipRect.Min;
    return (posVS + viewData.position) / viewData.zoom;
}

static float2 getNodeSpawnFromPopupPosition(ViewData const& viewData) {
    return screenToWorldSpace(viewData.popupPosition, viewData);
}

static Node* showNodeCreatePopup(NodeEditorDelegate* delegate,
                                 ViewData const& viewData) {
    if (!delegate) {
        return nullptr;
    }
    if (auto* node = delegate->createNodePopupMenu()) {
        ImGui::CloseCurrentPopup();
        float2 position = getNodeSpawnFromPopupPosition(viewData) -
                          float2(PaddedTitleHeight() / 2);
        node->setPosition(position);
        return node;
    }
    return nullptr;
}

Pin* getNewNodeTargetPin(Pin const& activePin, Node& node) {
    // clang-format off
    return visit(activePin, utl::overload{
        [&](OutputPin const&) -> Pin* {
            if (!node.inputs().empty()) {
                return node.input(0);
            }
            return nullptr;
        },
        [&](InputPin const&) -> Pin* {
            if (!node.outputs().empty()) {
                return node.output(0);
            }
            return nullptr;
        }
    }); // clang-format on
}

void Impl::draggingLinkReleasedPopup() {
    auto closeCallback = [this] { viewData.activePin = nullptr; };
    if (!beginPopup(viewData, DraggingLinkReleasedPopupName, closeCallback)) {
        return;
    }
    auto* node = showNodeCreatePopup(delegate.get(), viewData);
    if (node && viewData.activePin) {
        // clang-format off
        auto* otherPin = getNewNodeTargetPin(*viewData.activePin, *node);
        if (otherPin && isLinkable(*viewData.activePin, *otherPin)) {
            link(*viewData.activePin, *otherPin);
        }
        if (otherPin) {
            float2 pinPos =
                computePinPosition(*otherPin, otherPin->indexInParent());
            node->setPosition(node->position() - pinPos +
                              styleVars().PinAreaSize / 2);
        }
    }
    ImGui::EndPopup();
}

void Impl::backgroundPopup() {
    if (!beginPopup(viewData, BackgroundPopupName)) {
        return;
    }
    showDebugger |= ImGui::MenuItem("Show Debugger");
    showStyleEditor |= ImGui::MenuItem("Show Style Editor");
    ImGui::Separator();
    showNodeCreatePopup(delegate.get(), viewData);
    ImGui::EndPopup();
}

void Impl::displayPopups() {
    linkPopup();
    draggingLinkReleasedPopup();
    backgroundPopup();
}

void Impl::backgroundBehaviour() {
    bool haveNodeInteraction = viewData.activeNode || viewData.activePin;
    if (!viewData.windowHovered || haveNodeInteraction) {
        return;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !viewData.isSelectionDragging)
    {
        viewData.isSelectionDragging = true;
        viewData.selectionBegin = ImGui::GetMousePos();
        viewData.selectionEnd = ImGui::GetMousePos();
        selection.beginCandidates(getSelectionMode());
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        viewData.beginMoveDragging = true;
    }
    bool showPopup = ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
                     !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Right) &&
                     !viewData.isMoveDragging;
    if (showPopup) {
        if (viewData.hoveredLink.get()) {
            openPopup(LinkPopupName);
            viewData.popupLink = viewData.hoveredLink.get();
        }
        else {
            openPopup(BackgroundPopupName);
        }
    }
}

Impl::Impl(CommandPalette& cmd) {
    cmd.add("Delete Selected Nodes", { Key::Backspace, ModFlags::Control },
            [this] { deleteSelected(); });
    cmd.add("Select All", { Key::A, ModFlags::Control }, [this] {
        if (model) {
            selection.set(model->nodes());
        }
    });
    cmd.add("Copy Selected Nodes", { Key::C, ModFlags::Control }, [this] {
        if (!selection.empty()) {
            copyNodesToClipboard(selection);
        }
    });
    cmd.add("Cut Selected Nodes", { Key::X, ModFlags::Control }, [this] {
        if (!selection.empty()) {
            copyNodesToClipboard(selection);
        }
        deleteSelected();
    });
    cmd.add("Paste Nodes", { Key::V, ModFlags::Control }, [this] {
        pasteNodesFromClipboard();
    });
}

void NodeEditor::onInput(InputEvent& event) { impl->onInput(event); }

void Impl::onInput(InputEvent& event) {
    using enum InputEventMask;
    event.dispatch<MouseDragged>([&](MouseDragEvent event) {
        if (viewData.beginMoveDragging) {
            viewData.isMoveDragging = true;
            viewData.position -= event.offset;
        }
        if (viewData.isSelectionDragging) {
            // FIXME: This must be .locationInWindow when not using viewports
            viewData.selectionEnd = event.locationOnScreen;
            ImRect selectionRect = viewData.getSelectionRect();
            if (model) {
                auto candidates = model->nodes() |
                                  ranges::views::filter([&](Node const* node) {
                    return nodeOverlapsRect(viewData, *node, selectionRect);
                });
                selection.setCandidates(candidates |
                                        ranges::to<utl::hashset<Node*>>);
            }
        }
        return true;
    });
    event.dispatch<ScrollWheel>([&](ScrollEvent event) {
        if (!event.isTrackpad || viewData.disableBackgroundScrolling ||
            !viewData.clipRect.Contains(ImGui::GetMousePos()))
        {
            return false;
        }
        viewData.position -= event.offset;
        return true;
    });
    event.dispatch<Magnify>([&](MagnificationEvent event) {
        float zoom = viewData.zoom;
        float2 mouseLocWS =
            screenToWorldSpace(event.locationOnScreen, viewData);
        float newZoom = std::clamp(zoom * (1 + (float)event.offset),
                                   styleVars().MinZoom, styleVars().MaxZoom);
        float effectiveOffset = newZoom - zoom;
        viewData.position += mouseLocWS * effectiveOffset;
        viewData.zoom = newZoom;
        return true;
    });
}

NodeEditorGraph* NodeEditor::model() const { return impl->model; }

void NodeEditor::setModel(NodeEditorGraph* model) { impl->model = model; }

YAML::Node NodeEditor::serialize() const {
    YAML::Node root;
    root["ViewData"] = static_cast<PersistentViewData const&>(impl->viewData);
    return root;
}

void NodeEditor::deserialize(YAML::Node const& node) const {
    try {
        (PersistentViewData&)impl->viewData =
            node["ViewData"].as<PersistentViewData>();
    }
    catch (YAML::Exception const& err) {
        Logger::Error("Failed to deserialize node editor view data: ",
                      err.what());
    }
}

namespace {

struct PinDrawOptions {
    float4 color;
    bool missingInput;
    bool connected;
    bool hovered;
};

} // namespace

static void drawPin(float2 position, float zoom, PinDrawOptions options) {
    auto* DL = ImGui::GetWindowDrawList();
    float2 center = position + (styleVars().PinAreaSize / 2) * zoom;
    DL->AddCircleFilled(center, styleVars().PinRadius * zoom,
                        ImGui::ColorConvertFloat4ToU32(options.color));
    if (options.connected || options.missingInput) {
        float radius = (options.hovered || options.missingInput) ?
                           styleVars().PinConnectIndicatorRadiusHovered :
                           styleVars().PinConnectIndicatorRadius;
        float4 color = options.missingInput && !options.connected ?
                           styleColors().pinMissingIndicator :
                           styleColors().pinLinkIndicator;
        DL->AddCircleFilled(center, radius * zoom,
                            ImGui::ColorConvertFloat4ToU32(color));
    }
}

static float2 pinLabelPosition(Pin const& pin, float2 bgPadding,
                               float2 pinPosition, float2 textSize, float zoom,
                               float ypos) {
    // clang-format off
    return visit(pin, utl::overload{
        [&](InputPin const&) -> float2 {
            return { pinPosition.x - std::max(styleVars().PinInset, 1.0f) * zoom -
                         textSize.x - bgPadding.x,
                     ypos + pinPosition.y };
        },
        [&](OutputPin const&) -> float2 {
            return { pinPosition.x + (std::max(styleVars().PinInset, 1.0f) +
                         styleVars().PinAreaSize) * zoom + bgPadding.x,
                     ypos + pinPosition.y };
        },
    }); // clang-format on
}

static std::string_view selectPinLabel(Pin const& pin) {
    if (!displayPinUserType) {
        return pin.name();
    }
    static std::string tmpBuffer;
    tmpBuffer = utl::strcat((int)pin.userType());
    return tmpBuffer;
}

static void displayPinLabelOverlay(Pin const& pin, ViewData const& viewData) {
    std::string_view label = selectPinLabel(pin);
    if (label.empty()) {
        return;
    }
    auto* textBegin = label.data();
    auto* textEnd = textBegin + label.size();
    float2 pinPosition = computeScreenPinPosition(viewData, pin);
    auto* DL = ImGui::GetWindowDrawList();
    float2 textSize =
        (float2)ImGui::CalcTextSize(textBegin, textEnd) * viewData.zoom;
    float ypos = styleVars().PinAreaSize * viewData.zoom / 2 - textSize.y / 2;
    float2 bgPadding = 3 * viewData.zoom;
    float bgRounding = 3 * viewData.zoom;
    auto bgCol = ImGui::GetColorU32(ImGuiCol_PopupBg);
    float2 textPos = pinLabelPosition(pin, bgPadding, pinPosition, textSize,
                                      viewData.zoom, ypos);
    DL->AddRectFilled(textPos - bgPadding, textPos + textSize + bgPadding,
                      bgCol, bgRounding);
    DL->AddText(nullptr, ImGui::GetFontSize() * viewData.zoom, textPos,
                ImGui::GetColorU32(ImGuiCol_Text), textBegin, textEnd);
}

namespace {

/// Bezier curve discriptor for drawing a link. All points are in screen
/// coordinates
struct LinkDrawData {
    /// The start point of the link
    float2 begin;

    /// The end point of the link
    float2 end;

    /// The mid point between `begin` and `end`
    float2 mid;

    /// The size of the AABB spanned by `begin` and `end`
    float2 area;

    /// The bezier control point for the start point
    float2 beginControlPoint;

    /// The bezier control point for the end point
    float2 endControlPoint;
};

} // namespace

static LinkDrawData computeLinkDrawData(float2 begin, float2 end, float zoom) {
    LinkDrawData dd{};
    dd.begin = begin;
    dd.end = end;
    dd.mid = (begin + end) / 2;
    dd.area = vml::abs(end - begin);
    float crtlPointFactor =
        zoom * 2.0f *
        std::atan(dd.area.y / zoom /
                  styleVars().LinkBezierControlPointYFalloff) /
        vml::constants<float>::pi;
    float ctrlPointX =
        crtlPointFactor * std::clamp(dd.area.x,
                                     styleVars().LinkBezierControlPointXMin,
                                     styleVars().LinkBezierControlPointXMax);
    dd.beginControlPoint = dd.begin + float2(ctrlPointX, 0);
    dd.endControlPoint = dd.end - float2(ctrlPointX, 0);
    return dd;
}

static bool testPathOverlap(std::span<ImVec2 const> path, float2 point,
                            float dist) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
        float2 p1 = path[i];
        float2 p2 = path[i + 1];
        if (vml::distance(vml::line_segment_2D<>(p1, p2), point) <= dist) {
            return true;
        }
    }
    return false;
}

static bool testLinkOverlap(LinkDrawData const& dd,
                            std::span<ImVec2 const> path, float2 point,
                            float dist) {
    vml::AABB<float, 2> rect(dd.begin, dd.end, dd.beginControlPoint,
                             dd.endControlPoint);
    vml::AABB<float, 2> wideRect(rect.lower_bound() - float2(dist),
                                 rect.upper_bound() + float2(dist));
    if (!vml::do_intersect(wideRect, point)) {
        return false;
    }
    return testPathOverlap(path, point, dist);
}

/// Draws a link from \p begin to \p end in screen coordinates
/// \Returns `true` if \p mousePos overlaps the link
static bool displayLink(float2 begin, float2 end, float4 color,
                        std::optional<float2> mousePos, bool mayHighlight,
                        bool mustHighlight, float zoom) {
    auto* DL = ImGui::GetWindowDrawList();
    auto dd = computeLinkDrawData(begin, end, zoom);
    DL->PathLineTo(begin);
    DL->PathBezierQuadraticCurveTo(dd.beginControlPoint, dd.mid, 0);
    DL->PathBezierQuadraticCurveTo(dd.endControlPoint, end, 0);
    auto col = ImGui::ColorConvertFloat4ToU32(color);
    bool hovered = mousePos &&
                   testLinkOverlap(dd, DL->_Path, *mousePos,
                                   styleVars().LinkHoverDist * zoom);
    bool drawHighlighted = (mayHighlight && hovered) || mustHighlight;
    float width = drawHighlighted ? styleVars().LinkWidthHovered :
                                    styleVars().LinkWidth;
    DL->PathStroke(col, ImDrawFlags_None, width * zoom);
    return hovered;
}

static void drawLink(float2 begin, float2 end, float4 color, bool highlighted,
                     float zoom) {
    displayLink(begin, end, color, std::nullopt, /* mayHighlight: */ true,
                /* mustHighlight: */ highlighted, zoom);
}

void Impl::displayLinks(Node& node) {
    float2 nodepos = (float2)ImGui::GetWindowPos() +
                     node.position() * viewData.zoom - viewData.position;
    ImGui::PushID("Inputs");
    for (auto [index, pin]: node.inputs() | enumerate) {
        displayPinLink(*pin, nodepos + computePinPosition(*pin, index) *
                                           viewData.zoom);
    }
    ImGui::PopID();
}

void Impl::displayPinLink(InputPin& pin, float2 pinPosition) {
    auto* origin = pin.origin();
    if (!origin) {
        return;
    }
    Link link = { origin, &pin };
    float2 begin = computeScreenPinPosition(viewData, *origin) +
                   styleVars().PinAreaSize * viewData.zoom / 2;
    float2 end = pinPosition + styleVars().PinAreaSize * viewData.zoom / 2;
    float4 color = getLinkColor(origin->userType());
    bool mayHighlight = viewData.hoveredLink.getLast() == link;
    bool mustHighlight = link.contains(viewData.hoveredPin.getLast()) &&
                         !viewData.activePin;
    bool hovered = displayLink(begin, end, color, ImGui::GetMousePos(),
                               mayHighlight, mustHighlight, viewData.zoom);
    if (hovered && viewData.windowOrChildHovered && !viewData.activePin) {
        viewData.hoveredLink.set(link);
        viewData.hoveredLinkUnderNode.set(link);
    }
}

static bool isDraggingLinkVisible(ViewData const& viewData) {
    bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
                     ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    bool havePopup = viewData.activePopupName ==
                     (void*)DraggingLinkReleasedPopupName;
    return viewData.activePin && (mouseDown || havePopup);
}

void Impl::displayDraggingLink() {
    if (!isDraggingLinkVisible(viewData)) {
        return;
    }
    float2 pos1 = computeScreenPinPosition(viewData, *viewData.activePin) +
                  styleVars().PinAreaSize * viewData.zoom / 2;
    float2 pos2 = viewData.activePopupName ==
                          (void*)DraggingLinkReleasedPopupName ?
                      viewData.popupPosition :
                      (float2)ImGui::GetMousePos();
    if (isa<InputPin>(viewData.activePin)) {
        std::swap(pos1, pos2);
    }
    float4 color = getLinkColor(viewData.activePin->userType());
    drawLink(pos1, pos2, color, /* highlighted: */ true, viewData.zoom);
}

void Impl::pinPopup(Pin& pin, bool show) {
    static constexpr auto name = "Pin_Popup";
    if (show) {
        ImGui::OpenPopup(name);
    }
    if (!ImGui::BeginPopup(name)) {
        return;
    }
    if (ImGui::MenuItem("Clear Links")) {
        clearLinksWithCallback(pin);
    }
    ImGui::EndPopup();
}

namespace {

enum class TextAlign { Left, Right };

} // namespace

static void displayPinLabelInBody(std::string_view label, float2 position,
                                  TextAlign align, float zoom,
                                  float& maxLabelWidth) {
    char const* begin = label.data();
    char const* end = begin + label.size();
    float2 textSize = (float2)ImGui::CalcTextSize(begin, end) * zoom;
    position.y -= textSize.y / 2;
    if (align == TextAlign::Right) {
        position.x -= textSize.x;
    }
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddText(nullptr, ImGui::GetFontSize() * zoom, position,
                ImGui::ColorConvertFloat4ToU32(styleColors().nodeBodyText),
                begin, end);
    maxLabelWidth = std::max(maxLabelWidth, textSize.x);
}

static bool isOptional(Pin const& pin) {
    return test(pin.desc().flags & PinFlags::Optional);
}

static float4 selectPinColor(Pin const& pin) {
    if (pin.desc().color) {
        return *pin.desc().color;
    }
    if (isOptional(pin)) {
        return styleColors().pinOptBG;
    }
    return styleColors().pinBG;
}

static bool inputMissing(Pin const& pin) {
    if (isOptional(pin)) {
        return false;
    }
    auto* input = dyncast<InputPin const*>(&pin);
    if (!input) {
        return false;
    }
    return input->origin() == nullptr;
}

static bool isConnected(Pin const& pin) {
    // clang-format off
    return visit(pin, utl::overload{
        [](InputPin const& pin) { return pin.origin() != nullptr; },
        [](OutputPin const& pin) { return !pin.targets().empty(); },
    }); // clang-format on
}

static float2 pinInBodyLabelPosition(Pin const& pin, float2 pinPosition,
                                     float zoom) {
    float areaSize = styleVars().PinAreaSize * zoom;
    // clang-format off
    return pinPosition + visit(pin, utl::overload{
        [&](InputPin const&) { return float2(areaSize, areaSize / 2); },
        [&](OutputPin const&) { return float2(0,  areaSize / 2); },
    }); // clang-format on
}

void Impl::displayPin(Pin& pin, float2 position, float& maxLabelWith,
                      bool displayLabelInBody) {
    ImGui::PushID(&pin);
    float areaSize = styleVars().PinAreaSize * viewData.zoom;
    ImRect bb(position, position + float2(areaSize));
    ImGui::ItemSize(float2(areaSize));
    auto ID = ImGui::GetID("Pin_Handle");
    ImGui::ItemAdd(bb, ID);
    bool hovered = ImGui::ItemHoverable(bb, ID, ImGuiItemFlags_AllowOverlap);
    bool clickedLeft = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool clickedRight = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (clickedLeft || clickedRight) {
        viewData.activePin = &pin;
        if (ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 2) {
            clearLinksWithCallback(pin);
        }
    }
    pinPopup(pin, /* show: */ clickedRight);
    bool isDraggingCompatiblePin = viewData.activePin &&
                                   isLinkable(*viewData.activePin, pin);
    bool drawConnected =
        isConnected(pin) || (hovered && isDraggingCompatiblePin) ||
        (viewData.activePin == &pin && isDraggingLinkVisible(viewData));
    bool drawHovered =
        (hovered && (!viewData.activePin || isDraggingCompatiblePin)) ||
        viewData.hoveredLink.get().contains(&pin) ||
        (!viewData.activePin && viewData.hoveredPin.getLast() &&
         isLinked(pin, *viewData.hoveredPin.getLast())) ||
        viewData.activePin == &pin;
    drawPin(position, viewData.zoom,
            { .color = selectPinColor(pin),
              .missingInput = inputMissing(pin) && viewData.activePin != &pin,
              .connected = drawConnected,
              .hovered = drawHovered });
    if (displayLabelInBody) {
        displayPinLabelInBody(selectPinLabel(pin),
                              pinInBodyLabelPosition(pin, position,
                                                     viewData.zoom),
                              isa<InputPin>(pin) ? TextAlign::Left :
                                                   TextAlign::Right,
                              viewData.zoom, maxLabelWith);
    }
    if (hovered) {
        viewData.hoveredPin.set(&pin);
    }
    if (hovered && isDraggingCompatiblePin &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        link(*viewData.activePin, pin);
    }
    ImGui::PopID();
}

static float2 computeNodeMinSize(Node const& node) {
    float2 bodyPadding = NodeBodyPadding();
    float pinHeight = styleVars().PinAreaSize *
                      std::max(node.inputs().size(), node.outputs().size());
    float2 preferredSize = node.controller() ?
                               node.controller()->preferredContentSize() :
                               float2();
    float2 contentSize = vml::max(preferredSize, float2(0, pinHeight));
    contentSize += float2(0, styleVars().NodeBodyTitleHeight + bodyPadding.y);
    float2 result =
        vml::max(node.desc().minSize, node.layoutData().minSize, contentSize);
    result.y += styleVars().NodeBodyBottomPadding + 2 * bodyPadding.y;
    float leftInset = node.inputs().empty() ? bodyPadding.x :
                                              styleVars().PinAreaSize;
    float rightInset = node.outputs().empty() ? bodyPadding.x :
                                                styleVars().PinAreaSize;
    result.x += leftInset + rightInset;
    return result;
}

static bool alwaysDisplayInputNames(Node const& node) {
    return !node.controller() || node.controller()->alwaysDisplayInputNames();
}

static bool alwaysDisplayOutputNames(Node const& node) {
    return node.controller() && node.controller()->alwaysDisplayOutputNames();
}

static bool alwaysDisplayPinNames(Node const& node, PinType type) {
    switch (type) {
    case PinType::InputPin:
        return alwaysDisplayInputNames(node);
    case PinType::OutputPin:
        return alwaysDisplayOutputNames(node);
    default:
        BL_UNREACHABLE();
    }
}

void Impl::displayPins(Node& node) {
    float2 nodepos =
        (float2)ImGui::GetWindowPos() + NodeOuterPadding() * viewData.zoom;
    float maxInputLabelWidth = 0;
    float maxOutputLabelWidth = 0;
    {
        ImGui::PushID("Inputs");
        bool displayInBody = alwaysDisplayInputNames(node);
        for (auto [index, pin]: node.inputs() | enumerate) {
            displayPin(*pin,
                       nodepos +
                           computePinPosition(*pin, index) * viewData.zoom,
                       maxInputLabelWidth, displayInBody);
        }
        ImGui::PopID();
    }
    {
        ImGui::PushID("Outputs");
        bool displayInBody = alwaysDisplayOutputNames(node);
        for (auto [index, pin]: node.outputs() | enumerate) {
            float2 pinpos = computePinPosition(*pin, index) * viewData.zoom;
            displayPin(*pin, nodepos + pinpos, maxOutputLabelWidth,
                       displayInBody);
        }
        ImGui::PopID();
    }
    node.layoutData().minSize.x =
        (maxInputLabelWidth + maxOutputLabelWidth) / viewData.zoom;
}

static float2 computeLabelSize(char const* label, char const* labelEnd,
                               float2 scaledSize, float zoom) {
    float2 desiredLabelSize =
        (float2)ImGui::CalcTextSize(label, labelEnd) * zoom;
    float2 availLabelSize =
        scaledSize -
        float2(2 * (styleVars().NodeBodySelectionOutlineInset + 1), 0) * zoom;
    return vml::min(desiredLabelSize, availLabelSize);
}

/// \Returns the area in which content can be drawn
static ImRect drawNodeBody(Node const& node, float zoom, bool selected) {
    auto* DL = ImGui::GetWindowDrawList();
    auto* font =
        FontManager::get(FontDesc::UIDefault().setWeight(FontWeight::Bold));
    ImGui::PushFont(font);
    float2 pos = (float2)ImGui::GetWindowPos() + NodeOuterPadding() * zoom;
    float2 scaledSize = node.size() * zoom;
    float2 bodyPadding = NodeBodyPadding() * zoom;
    auto* label = node.name().data();
    auto* labelEnd = node.name().data() + node.name().size();
    float2 labelSize = computeLabelSize(label, labelEnd, scaledSize, zoom);
    float2 labelPos =
        pos + float2((scaledSize.x - labelSize.x) / 2, bodyPadding.y);
    float rounding = styleVars().NodeBodyRounding * zoom;
    /// Selection outline
    if (selected) {
        float inset = styleVars().NodeBodySelectionOutlineInset * zoom;
        float inset2 = inset + 1 * zoom;
        DL->AddRect(pos - inset2, pos + scaledSize + inset2,
                    IM_COL32(0, 0, 0, 127), rounding + inset2, ImDrawFlags_None,
                    2 * inset2);
        DL->AddRect(pos - inset, pos + scaledSize + inset,
                    ImGui::ColorConvertFloat4ToU32(
                        styleColors().nodeSelectedOutline),
                    rounding + inset, ImDrawFlags_None, 2 * inset);
    }
    /// Title bar
    float2 titleBarSize = float2(scaledSize.x, labelSize.y + 2 * bodyPadding.y);
    DL->AddRectFilled(pos, pos + titleBarSize,
                      ImGui::ColorConvertFloat4ToU32(node.color()), rounding,
                      ImDrawFlags_RoundCornersTop);
    /// Content BG
    float2 contentPos = pos + float2(0, titleBarSize.y);
    DL->AddRectFilled(contentPos, pos + scaledSize,
                      ImGui::ColorConvertFloat4ToU32(styleColors().nodeBody),
                      rounding, ImDrawFlags_RoundCornersBottom);
    /// Outlines
    float highlightOutlineInset = 1 * zoom;
    DL->AddRect(pos, pos + scaledSize,
                ImGui::ColorConvertFloat4ToU32(styleColors().blackOutline),
                rounding, ImDrawFlags_None, 1 * zoom);
    DL->AddLine(pos + float2(0, titleBarSize.y), pos + titleBarSize,
                ImGui::ColorConvertFloat4ToU32(styleColors().blackOutline),
                1 * zoom);
    DL->AddRect(pos + highlightOutlineInset,
                pos + titleBarSize - highlightOutlineInset,
                ImGui::ColorConvertFloat4ToU32(styleColors().whiteOutline),
                rounding - highlightOutlineInset, ImDrawFlags_RoundCornersTop,
                1 * zoom);
    DL->AddRect(contentPos + highlightOutlineInset,
                pos + scaledSize - highlightOutlineInset,
                ImGui::ColorConvertFloat4ToU32(styleColors().whiteOutline *
                                               float4(1, 1, 1, 0.5)),
                rounding - highlightOutlineInset,
                ImDrawFlags_RoundCornersBottom, 1 * zoom);
    /// Label
    auto labelCol = ImGui::ColorConvertFloat4ToU32(styleColors().nodeTitleText);
    ImGui::PushClipRect(labelPos, labelPos + labelSize, true);
    DL->AddText(nullptr, ImGui::GetFontSize() * zoom, labelPos, labelCol, label,
                labelEnd);
    ImGui::PopClipRect();
    ImGui::PopFont();

    float2 contentRectMin =
        pos + (bodyPadding + float2(0, labelSize.y + 2 * bodyPadding.y));
    float2 contentRectMax = pos + scaledSize - bodyPadding;
    if (!node.inputs().empty()) {
        contentRectMin.x =
            contentRectMin.x - bodyPadding.x + styleVars().PinAreaSize * zoom;
    }
    if (!node.outputs().empty()) {
        contentRectMax.x =
            contentRectMax.x + bodyPadding.x - styleVars().PinAreaSize * zoom;
    }
    return ImRect(contentRectMin, contentRectMax);
}

/// \Returns true if the background has been clicked with the left or right
/// mouse button
static bool displayNodeContent(ViewData& viewData, ImRect contentRect,
                               Node const& node) {
    if (!beginInvisibleWindow("Content", contentRect.Min,
                              contentRect.GetSize()))
    {
        ImGui::EndChild();
        return false;
    }
    ImGui::SetNextItemAllowOverlap();
    bool bgPressed =
        ImGui::InvisibleButton("Content_View_Background_Button",
                               ImGui::GetContentRegionAvail(),
                               ImGuiButtonFlags_PressedOnClick |
                                   ImGuiButtonFlags_MouseButtonLeft |
                                   ImGuiButtonFlags_MouseButtonRight);
    ImGui::SetCursorPos({});
    if (node.controller()) {
        node.controller()->bodyContent();
        auto* window = ImGui::GetCurrentWindowRead();
        if (ImGui::IsWindowHovered() && (float2)window->ScrollMax != float2{}) {
            viewData.disableBackgroundScrolling = true;
        }
    }
    ImGui::EndChild();
    return bgPressed;
}

static void displayNodeResizeGrip(ViewData& viewData, Node& node) {
    float2 areaSize = { 10, 10 };
    float2 pos = (float2)ImGui::GetWindowPos() +
                 NodeOuterPadding() * viewData.zoom +
                 node.size() * viewData.zoom - areaSize;
    ImGuiID id = ImGui::GetID("Node_Resize_Grip");
    ImRect bb(pos, pos + areaSize);
    ImGui::ItemSize(areaSize);
    ImGui::ItemAdd(bb, id);
    bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
    if (ImGui::IsItemClicked()) {
        viewData.activeResize = &node;
        viewData.resizeSize = node.size() * viewData.zoom;
    }
    bool dragging = viewData.activeResize == &node &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragging) {
        float2 minSize = computeNodeMinSize(node);
        viewData.resizeSize += ImGui::GetIO().MouseDelta;
        float2 newSize = vml::max(viewData.resizeSize / viewData.zoom, minSize);
        node.setSize(newSize);
    }
    if (hovered || dragging) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
    }
}

void Impl::nodeBodyPopup(Node& node, bool show) {
    static constexpr auto name = "Node_Popup";
    if (show) {
        ImGui::OpenPopup(name);
    }
    if (!ImGui::BeginPopup(name)) {
        return;
    }
    BL_ASSERT(selection.contains(&node),
              "Popup should not open on unselected node");
    if (ImGui::MenuItem(selection.size() == 1 ? "Delete Node" :
                                                "Delete Selected Nodes"))
    {
        defer([this] { deleteSelected(); });
    }
    if (auto* controller = node.controller()) {
        controller->contextMenuContent();
    }
    ImGui::EndPopup();
}

bool Impl::canDropNodeIntoLink(Node const& node, Link link) const {
    if (node.inputs().size() != 1 || node.outputs().size() != 1) {
        return false;
    }
    InputPin const* in = node.inputs().front();
    OutputPin const* out = node.outputs().front();
    return !in->origin() && out->targets().empty() &&
           isLinkable(*link.origin, *in) && isLinkable(*out, *link.dest);
}

void Impl::dropNodeIntoLink(Node& node, Link link) {
    BL_ASSERT(canDropNodeIntoLink(node, link));
    InputPin* in = node.inputs().front();
    OutputPin* out = node.outputs().front();
    deleteLink(link);
    ::link(*link.origin, *in);
    ::link(*out, *link.dest);
}

template <typename T, size_t Dim, vml::vector_options O>
static vml::AABB<T, Dim, O> union_(vml::AABB<T, Dim, O> a,
                                   vml::AABB<T, Dim, O> b) {
    return vml::AABB<T, Dim, O>(vml::min(a.lower_bound(), b.lower_bound()),
                                vml::max(a.upper_bound(), b.upper_bound()));
}

void Impl::displayNodeBody(Node& node) {
    float2 padding = NodeOuterPadding() * viewData.zoom;
    float2 pos = (float2)ImGui::GetWindowPos() - viewData.position +
                 node.position() * viewData.zoom;
    float2 windowPos = pos - padding;
    float2 scaledNodeSize = node.size() * viewData.zoom;
    float2 windowSize = scaledNodeSize + 2 * padding;
    bool open = beginInvisibleWindow("Body", windowPos, windowSize);
    if (!open && viewData.activeNode != &node) {
        ImGui::EndChild();
        return;
    }
    auto* DL = ImGui::GetWindowDrawList();
    DL->PushClipRect(viewData.clipRect.Min, viewData.clipRect.Max);
    bool isSelected = selection.contains(&node);
    ImRect contentRect = drawNodeBody(node, viewData.zoom, isSelected);
    ImGuiID id = ImGui::GetID("Node_Body");
    ImRect bb(pos, pos + scaledNodeSize);
    ImGui::ItemSize(scaledNodeSize);
    ImGui::ItemAdd(bb, id);
    vml::rectangle<float> nodeRect(node.position(),
                                   node.position() + node.size());
    viewData.nodeRectUnion = vml::area(viewData.nodeRectUnion) == 0 ?
                                 nodeRect :
                                 union_(viewData.nodeRectUnion, nodeRect);
    if (ImGui::IsMouseHoveringRect(windowPos + 2, windowPos + windowSize - 4)) {
        viewData.hoveredLink.set({});
        viewData.hoveredNode.set(&node);
    }
    bool bgClickedLeft = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool bgClickedRight = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    displayPins(node);
    if (displayNodeContent(viewData, contentRect, node)) {
        bgClickedLeft |= ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bgClickedRight |= ImGui::IsMouseDown(ImGuiMouseButton_Right);
    }
    if (node.desc().resizable) {
        displayNodeResizeGrip(viewData, node);
    }
    if (bgClickedLeft || bgClickedRight) {
        defer([this, node = &node] { orderFront(node); });
        viewData.activeNode = &node;
    }
    /// Selected nodes change selection state when mouse is released
    if (isSelected && viewData.activeNode == &node &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !viewData.draggingNode && !viewData.handledSelectionSinceMousePressed)
    {
        selection.amend(&node, getSelectionMode());
    }
    /// True if ...
    bool haveSecondaryInteraction = viewData.activeNode != &node ||
                                    viewData.activeResize || viewData.activePin;
    /// Not selected nodes change selection state when mouse is pressed
    if (!isSelected && (bgClickedLeft || bgClickedRight) &&
        !haveSecondaryInteraction)
    {
        /// We use this flag so we don't select and immediately unselect when
        /// the mouse is released
        viewData.handledSelectionSinceMousePressed = true;
        selection.amend(&node, getSelectionMode());
    }
    nodeBodyPopup(node,
                  /* show: */ bgClickedRight && !haveSecondaryInteraction);
    bool dragging = [&] {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            return false;
        }
        return !haveSecondaryInteraction;
    }();
    Link hoveredLinkBelow = viewData.hoveredLinkUnderNode.get();
    if (hoveredLinkBelow && viewData.draggingNode == &node &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        canDropNodeIntoLink(node, hoveredLinkBelow))
    {
        dropNodeIntoLink(node, hoveredLinkBelow);
    }
    if (dragging) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            viewData.draggingNode = &node;
        }
        float2 delta = ImGui::GetIO().MouseDelta;
        for (auto* node: selection.elements()) {
            node->setPosition(node->position() + delta / viewData.zoom);
        }
        if (hoveredLinkBelow && canDropNodeIntoLink(node, hoveredLinkBelow)) {
            viewData.hoveredLink.set(hoveredLinkBelow);
        }
    }
    DL->PopClipRect();
    ImGui::EndChild();
}

void Impl::displayNode(Node& node) {
    if (node.desc().resizable) {
        node.setSize(vml::max(node.size(), computeNodeMinSize(node)));
    }
    else {
        node.setSize(computeNodeMinSize(node));
    }
    ImGui::PushID(&node);
    displayNodeBody(node);
    ImGui::PopID();
}

void NodeEditor::display() { impl->display(); }

void Impl::handleDraggingLinkMouseReleased() {
    if (ImGui::IsPopupOpen(ImGuiID(), ImGuiPopupFlags_AnyPopupId)) {
        return;
    }
    bool shallOpenDraggingLinkMenu =
        viewData.activePin && !viewData.hoveredPin.get() &&
        !viewData.hoveredNode.get() && !viewData.activePopupName;
    if (shallOpenDraggingLinkMenu) {
        openPopup(DraggingLinkReleasedPopupName);
    }
    else if (viewData.activePopupName != (void*)DraggingLinkReleasedPopupName) {
        viewData.activePin = nullptr;
    }
}

void Impl::display() {
    auto flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs;
    if (!ImGui::BeginChild("Node_Editor", {}, ImGuiChildFlags_None, flags)) {
        ImGui::EndChild();
        return;
    }
    viewData.beginFrame();
    drawBackground(viewData, viewData.zoom);
    bool isMouseOverWindowRect =
        ImGui::GetCurrentWindow()->Rect().Contains(ImGui::GetMousePos());
    viewData.windowHovered =
        isMouseOverWindowRect &&
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow |
                               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    viewData.windowOrChildHovered =
        isMouseOverWindowRect &&
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow |
                               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                               ImGuiHoveredFlags_ChildWindows);
    if (model) {
        for (auto* node: model->nodes()) {
            displayLinks(*node);
        }
        displayDraggingLink();
        viewData.nodeRectUnion = {};
        for (auto* node: model->nodes()) {
            displayNode(*node);
        }
    }
    backgroundBehaviour();
    drawOverlays();
    displayPopups();
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        handleDraggingLinkMouseReleased();
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        viewData.resetActiveData();
        selection.applyCandidates();
    }
    ranges::for_each(deferList, ranges::invoke);
    deferList.clear();
    ImGui::EndChild();

    if (showDebugger) {
        displayDebugger(viewData, selection.elements(), &showDebugger);
    }
    if (showStyleEditor) {
        styleEditor(&showStyleEditor);
    }
}

static bool needDrawLabelOverlay(ViewData const& viewData,
                                 Pin const& hoveredPin) {
    auto& node = hoveredPin.parent();
    if (alwaysDisplayPinNames(node, hoveredPin.type())) {
        return false;
    }
    if (!viewData.activePin) {
        return true;
    }
    return isIOCompatible(hoveredPin, *viewData.activePin);
}

static void drawSelectionRect(ImRect rect) {
    auto size = rect.GetSize();
    if (size.x <= 1 || size.y <= 1) {
        return;
    }
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(rect.Min, rect.Max,
                      ImGui::ColorConvertFloat4ToU32(
                          styleColors().selectionRect));
    DL->AddRect(rect.Min, rect.Max,
                ImGui::ColorConvertFloat4ToU32(
                    styleColors().selectionRectOutline));
}

void Impl::drawOverlays() {
    if (!beginInvisibleWindow("Overlays", ImGui::GetWindowPos(),
                              ImGui::GetWindowSize(),
                              ImGuiWindowFlags_NoInputs))
    {
        ImGui::EndChild();
        return;
    }
    if (auto* pin = viewData.hoveredPin.get();
        pin && needDrawLabelOverlay(viewData, *pin))
    {
        displayPinLabelOverlay(*pin, viewData);
    }
    if (viewData.isSelectionDragging) {
        drawSelectionRect(viewData.getSelectionRect());
    }
    ImGui::EndChild();
}

void Impl::orderFront(Node* node) {
    if (model) {
        model->orderFront(node);
    }
}

void Impl::deleteSelected() {
    if (!model) {
        return;
    }
    auto pred = [&](Node const& node) { return selection.contains(&node); };
    model->deleteIf(pred);
    selection.clear();
}

void Impl::copyNodesToClipboard(std::span<Node const* const> nodes) {
    YAML::Emitter out;
    out << serializeNodes(nodes, { .generateNewIDs = true,
                                   .serializeLinksToOtherNodes = false });
    ImGui::SetClipboardText(out.c_str());
}

void Impl::pasteNodesFromClipboard() {
    if (!model || !delegate) {
        return;
    }
    auto controllerFactory = [this](YAML::Node const& node) {
        return delegate->deserializeNodeController(node);
    };
    auto transform = [](Node& node) {
        node.setPosition(node.position() + float2(50));
    };
    YAML::Node source;
    try {
        source = YAML::Load(ImGui::GetClipboardText());
    }
    catch (YAML::Exception const& e) {
        Logger::Error("Failed to paste nodes from clipboard: ", e.what());
    }
    auto nodes = deserializeNodes(*model, source, controllerFactory, transform);
    selection.set(nodes);
}

void Impl::openPopup(char const* name) {
    ImGui::OpenPopup(name);
    viewData.popupPosition = ImGui::GetMousePos();
    viewData.activePopupName = name;
}

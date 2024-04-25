#include "Poppy/Views/NodeEditor.h"

#include <functional>
#include <optional>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/overload.hpp>
#include <utl/stack.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/EnumCount.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Core/Yaml.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Font.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

/// # Style

struct NodeEditorStyleColors {
    float4 whiteOutline;
    float4 blackOutline;
    float4 nodeBodyText;
    float4 pinBG;
    float4 pinOptBG;
    float4 pinMissingIndicator;
    float4 pinLinkIndicator;
    float4 link;
    float4 selectionRect;
    float4 selectionRectOutline;
};

struct NodeEditorStyleVars {
    float LinkWidth = 3;
    float LinkWidthHovered = 5;
    float LinkHoverDist = 7;
    /// The side length of the square that is occupied by a pin. This is the
    /// interaction area, the rendered size is smaller
    float PinAreaSize = 20;
    float PinRadius = 6;
    float NodeBodyTitleHeight = 20;
    /// Additional bottom padding because it looks better
    float NodeBodyBottomPadding = 3;
    float NodeBodyRounding = 10;
    float NodeBodySelectionOutlineInset = 3;
    float MissingInputIndicatorWidth = 3;
    /// How far the pins are moved into the node. If this is zero the outer edge
    /// of the node aligns with the edge of the pin
    float PinInset = 0;
    float2 MinNodeBodyPadding = 5;
    float2 BackgroundLineDist = { 30, 30 };
    float LinkBezierControlPointXMin = 100;
    float LinkBezierControlPointXMax = 100;
    float LinkBezierControlPointYFalloff = 150;
};

struct NodeEditorStyle {
    EnumArray<AppearanceType, NodeEditorStyleColors> colors;
    NodeEditorStyleVars vars;

    NodeEditorStyle() {
        auto& light = colors[AppearanceType::Light];
        auto& dark = colors[AppearanceType::Dark];
        light.whiteOutline = { 1, 1, 1, 0.5 };
        light.blackOutline = { 0, 0, 0, 0.7 };
        light.nodeBodyText = { 1, 1, 1, 1 };
        light.pinBG = { 0.2, 0.2, 0.2, 1 };
        light.pinOptBG = { 0.4, 0.4, 0.4, 1 };
        light.pinMissingIndicator = { 1, 0, 0.2, 1 };
        light.link = { 0, 0, 0, 0.6 };
        light.pinLinkIndicator = { 1, 1, 1, 0.8 };
        light.selectionRect = { 0, 0, 0, 0.14 };
        light.selectionRectOutline = { 0, 0, 0, 0.3 };

        dark.whiteOutline = light.whiteOutline;
        dark.blackOutline = light.blackOutline;
        dark.nodeBodyText = light.nodeBodyText;
        dark.pinBG = light.pinBG;
        dark.pinOptBG = light.pinOptBG;
        dark.pinMissingIndicator = light.pinMissingIndicator;
        dark.link = { 1, 1, 1, 0.8 };
        dark.pinLinkIndicator = dark.link;
        dark.selectionRect = { 1, 1, 1, 0.3 };
        dark.selectionRectOutline = { 1, 1, 1, 0.3 };
    }
};

/// This is not const so we can edit the values with the style editor window
static NodeEditorStyle style;

static NodeEditorStyleColors const& styleColors() {
    return style.colors[Appearance::get().type()];
}

static NodeEditorStyleVars const& styleVars() { return style.vars; }

/// Derived properties
static float2 NodeBodyPadding() {
    return vml::max(styleVars().MinNodeBodyPadding,
                    float2(styleVars().PinAreaSize + styleVars().PinInset, 0));
}
static float2 NodeOuterPadding() {
    return vml::max(float2(1), float2(-styleVars().PinInset, 0));
}

static void styleEditor(bool* open) {
    auto& col = style.colors[Appearance::get().type()];
    auto& var = style.vars;
    ImGui::Begin("Node Editor Style Viewer", open);
    // clang-format off
    ImGui::SeparatorText("Style Colors");
    ImGui::ColorEdit4("White Outline", col.whiteOutline.data());
    ImGui::ColorEdit4("Black Outline", col.blackOutline.data());
    ImGui::ColorEdit4("Link", col.link.data());
    ImGui::ColorEdit4("Selection Rect", col.selectionRect.data());
    ImGui::ColorEdit4("Selection Rect Outline", col.selectionRectOutline.data());
    ImGui::ColorEdit4("Pin Background", col.pinBG.data());
    ImGui::ColorEdit4("Optional Pin Background", col.pinOptBG.data());
    ImGui::ColorEdit4("Missing Pin Indicator", col.pinMissingIndicator.data());
    ImGui::ColorEdit4("Pin Link Indicator", col.pinMissingIndicator.data());
    ImGui::SeparatorText("Style Vars");
    ImGui::SliderFloat("LinkWidth", &var.LinkWidth, 0, 10);
    ImGui::SliderFloat("LinkWidthHovered", &var.LinkWidthHovered, 0, 10);
    ImGui::SliderFloat("LinkHoverDist", &var.LinkHoverDist, 0, 20);
    ImGui::SliderFloat("PinAreaSize", &var.PinAreaSize, 0, 100);
    ImGui::SliderFloat("PinRadius", &var.PinRadius, 0, 100);
    ImGui::SliderFloat("NodeBodyTitleHeight", &var.NodeBodyTitleHeight, 0, 100);
    ImGui::SliderFloat("NodeBodyBottomPadding", &var.NodeBodyBottomPadding, 0, 100);
    ImGui::SliderFloat("NodeBodyRounding", &var.NodeBodyRounding, 0, 100);
    ImGui::SliderFloat("NodeBodySelectionOutlineInset", &var.NodeBodySelectionOutlineInset, 0, 100);
    ImGui::SliderFloat("MissingInputIndicatorWidth", &var.MissingInputIndicatorWidth, 0, 10);
    ImGui::SliderFloat("PinInset", &var.PinInset, -100, 100);
    ImGui::SliderFloat2("MinNodeBodyPadding", var.MinNodeBodyPadding.data(), 0, 50);
    ImGui::SliderFloat2("BackgroundLineDist", var.BackgroundLineDist.data(), 0, 200);
    ImGui::SliderFloat("LinkBezierControlPointXMin", &var.LinkBezierControlPointXMin, 0, var.LinkBezierControlPointXMax);
    ImGui::SliderFloat("LinkBezierControlPointXMax", &var.LinkBezierControlPointXMax, var.LinkBezierControlPointXMin, 1000);
    ImGui::SliderFloat("LinkBezierControlPointYFalloff", &var.LinkBezierControlPointYFalloff, 0, 1000);
    // clang-format on
    ImGui::End();
}

using Impl = NodeEditor::Impl;

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

void Pin::clearLinks() {
    // clang-format off
    visit(*this, utl::overload{
        [&](OutputPin& pin) {
            pin.clearTargets();
        },
        [&](InputPin& pin) {
            pin.setOrigin(nullptr);
        }
    }); // clang-format on
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

static YAML::Node serializeNode(Node const& node) {
    auto& desc = node.desc();
    YAML::Node root;
    root["ID"] = desc.ID;
    root["Name"] = desc.name;
    root["Position"] = desc.position;
    root["Size"] = desc.size;
    if (desc.controller) {
        root["Controller"] = desc.controller->serialize();
    }
    return root;
}

static YAML::Node serializeLink(OutputPin const& out, InputPin const& in) {
    YAML::Node root;
    root["FromID"] = out.parent().ID();
    root["FromIndex"] = out.indexInParent();
    root["ToID"] = in.parent().ID();
    root["ToIndex"] = in.indexInParent();
    return root;
}

YAML::Node NodeEditorGraph::serialize() const {
    YAML::Node root;
    auto nodeList = root["Nodes"];
    auto linkList = root["Links"];
    for (auto* node: nodes()) {
        nodeList.push_back(serializeNode(*node));
        for (auto* input: node->inputs()) {
            if (input->origin()) {
                linkList.push_back(serializeLink(*input->origin(), *input));
            }
        }
    }
    return root;
}

static std::unique_ptr<Node> deserializeNode(
    YAML::Node const& source,
    utl::function_view<std::unique_ptr<NodeController>(YAML::Node const&)>
        deserializeController) {
    NodeDesc desc{};
    desc.ID = source["ID"].as<utl::uuid>();
    desc.name = source["Name"].as<std::string>();
    desc.position = source["Position"].as<vml::float2>();
    desc.size = source["Size"].as<vml::float2>();
    if (deserializeController) {
        desc.controller = deserializeController(source["Controller"]);
    }
    return std::make_unique<Node>(std::move(desc));
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
    }
    if (toIndex >= toNode->inputs().size()) {
        Logger::Warn("Failed to deserialize link: input index out of range: ",
                     toIndex, " (on node ", toNode->name(), ")");
    }
    link(*fromNode->output(fromIndex), *toNode->input(toIndex));
}

bool NodeEditorGraph::deserialize(
    YAML::Node const& source,
    utl::function_view<std::unique_ptr<NodeController>(YAML::Node const&)>
        deserializeController) {
    try {
        _nodes.clear();
        utl::hashmap<utl::uuid, Node*> map;
        for (auto& nodeSource: source["Nodes"]) {
            auto node = deserializeNode(nodeSource, deserializeController);
            if (!utl::is_null(node->ID())) {
                map[node->ID()] = node.get();
            }
            add(std::move(node));
        }
        for (auto& linkSource: source["Links"]) {
            deserializeLink(linkSource, map);
        }
        return true;
    }
    catch (YAML::Exception const& e) {
        Logger::Error("Failed to deserialize: ", e.what());
        return false;
    }
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

enum class Axis { Horizontal, Vertical };

struct PersistentViewData {
    vml::float2 position = { 0.0, 0.0 };
    float zoom = 1.0;
};

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

    explicit operator bool() const { return origin || dest; }
};

static void deleteLink(Link link) { link.dest->clearLinks(); }

/// Data that persists between frames when performing some actions.
/// This is reset whenever a mouse button is released
struct ActiveViewData {
    /// True if a node is currently being dragged
    bool isDraggingNode = false;
    /// True if we already changed selection state since the mouse was pressed
    bool handledSelectionSinceMousePressed = false;
    /// True if we are selecting be dragged the background
    bool isSelectionDragging = false;
    bool beginMoveDragging = false;
    bool isMoveDragging = false;
    float2 resizeSize = 0;
    float2 selectionBegin = 0;
    float2 selectionEnd = 0;
    Node* activeNode = nullptr;
    Node* activeResize = nullptr;
    Pin* activePin = nullptr;

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
    HoverItem<Pin*> hoveredPin;
    HoverItem<Node*> hoveredNode;
    HoverItem<Link> hoveredLink;

    void update() {
        auto* window = ImGui::GetCurrentWindow();
        clipRect = window->ContentRegionRect;
        hoveredPin.update();
        hoveredNode.update();
        hoveredLink.update();
    }
};

struct ViewData: PersistentViewData, ActiveViewData, FrameViewData {
    Link popupLink = {};

    void beginFrame() { FrameViewData::update(); }

    void resetActiveData() {
        static_cast<ActiveViewData&>(*this) = ActiveViewData{};
    }
};

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

template <typename T>
static void debuggerItem(char const* name, HoverItem<T> item) {
    return debuggerItem(name, item.get());
}

static void viewDataDebugger(ViewData const& viewData, bool* open) {
#define ITEM(name) debuggerItem(#name, viewData.name)
    ImGui::Begin("View Data Viewer", open);
    ImGui::SeparatorText("Persistent Data");
    ITEM(position);
    ITEM(zoom);
    ImGui::SeparatorText("Active Data");
    ITEM(isDraggingNode);
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
    ImGui::SeparatorText("Frame Data");
    ITEM(hoveredPin);
    ITEM(hoveredNode);
    ITEM(hoveredLink);
    ImGui::End();
#undef ITEM
}

enum class SelectionMode {
    Replace, /// Default, existing selection is replaced
    Add,     /// Additive selection, new selection does not unselect existing
             /// selection
    Toggle   /// "Exclusive-or" selection. Items are selected iff not already
             /// selected
};

static SelectionMode getSelectionMode() {
    if (ImGui::IsKeyDown(ImGuiKey_ModSuper)) {
        return SelectionMode::Toggle;
    }
    if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
        return SelectionMode::Add;
    }
    return SelectionMode::Replace;
}

/// Selection manager
class Selection {
public:
    /// \Returns `true` if \p node is selected
    bool contains(Node const* node) const {
        if (!mode) {
            return _nodes.contains(node);
        }
        switch (*mode) {
        case SelectionMode::Replace:
            return _candidates.contains(node);
        case SelectionMode::Add:
            return _nodes.contains(node) || _candidates.contains(node);
        case SelectionMode::Toggle:
            return _nodes.contains(node) != _candidates.contains(node);
        }
    }

    /// Calls `set`, `add` or `toggle` depending on the held modifier keys
    void amend(Node* node) {
        switch (getSelectionMode()) {
        case SelectionMode::Replace:
            set(node);
            break;
        case SelectionMode::Add:
            add(node);
            break;
        case SelectionMode::Toggle:
            toggle(node);
            break;
        }
    }

    /// Adds \p node to the selection
    void add(Node* node) { _nodes.insert(node); }

    /// \overload for ranges
    template <typename Itr, typename Sentinel>
    void add(Itr first, Sentinel last) {
        _nodes.insert(first, last);
    }

    /// If \p node is not selection, adds \p node to the selection. Otherwise
    /// removes \p node from the selection
    bool toggle(Node* node) {
        auto itr = _nodes.find(node);
        if (itr != _nodes.end()) {
            _nodes.erase(itr);
            return false;
        }
        else {
            add(node);
            return true;
        }
    }

    /// Clears the selection and selects \p node
    void set(Node* node) { set(&node, &node + 1); }

    /// \overload for ranges
    template <typename Itr, typename Sentinel>
    void set(Itr first, Sentinel last) {
        clear();
        add(first, last);
    }

    /// \overload for ranges
    void set(ranges::range auto&& rng) {
        set(ranges::begin(rng), ranges::end(rng));
    }

    /// Unselectes all nodes
    void clear() { _nodes.clear(); }

    /// \Returns a view over all selected nodes
    std::span<Node* const> nodes() const { return _nodes.values(); }

    /// \Returns the number of selected nodes
    size_t size() const { return _nodes.size(); }

    /// Begin rectangle selection
    void beginCandidates() { this->mode = getSelectionMode(); }

    /// Update rectangle selection
    void setCandidates(utl::hashset<Node*> nodes) {
        _candidates = std::move(nodes);
    }

    /// Finish rectangle selection
    void applyCandidates() {
        if (!mode) {
            return;
        }
        switch (*mode) {
        case SelectionMode::Replace:
            _nodes = std::move(_candidates);
            break;
        case SelectionMode::Add:
            _nodes.insert(_candidates.begin(), _candidates.end());
            _candidates.clear();
            break;
        case SelectionMode::Toggle: {
            for (auto* node: _candidates) {
                if (_nodes.contains(node)) {
                    _nodes.erase(node);
                }
                else {
                    _nodes.insert(node);
                }
            }
            break;
        }
        }
        mode = std::nullopt;
    }

private:
    utl::hashset<Node*> _nodes;
    utl::hashset<Node*> _candidates;
    std::optional<SelectionMode> mode;
};

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

    void linkPopup(bool show, Link link);
    void backgroundPopup(bool show);
    void backgroundBehaviour();
    void drawOverlays();
    void displayLinks(Node& node);
    void pinPopup(Pin& pin, bool show);
    void displayPin(Pin& pin, float2 position, float& maxLabelWith,
                    bool displayLabelInBody);
    void displayPins(Node& node);
    void nodeBodyPopup(Node& node, bool show);
    void displayNodeBody(Node& node);
    void displayNode(Node& node);

    void orderFront(Node* node);

    void deleteSelected();

    void defer(auto f) { deferList.push_back(f); }

    NodeEditorGraph model;
    Selection selection;
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

void NodeEditor::setGraph(NodeEditorGraph model) {
    impl->model = std::move(model);
}

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

static void drawBackground(float2 viewPos) {
    float4 color = ImGui::GetStyleColorVec4(ImGuiCol_Separator);
    float2 begin = vml::fmod(viewPos, styleVars().BackgroundLineDist);
    drawLines(begin.x, ImGui::GetWindowWidth(),
              styleVars().BackgroundLineDist.x, Axis::Horizontal, color);
    drawLines(begin.y, ImGui::GetWindowHeight(),
              styleVars().BackgroundLineDist.y, Axis::Vertical, color);
}

static bool nodeOverlapsRect(ViewData const& viewData, Node const& node,
                             ImRect rect) {
    auto nodeRect = toRect(node);
    nodeRect.Translate((float2)viewData.clipRect.Min + viewData.position);
    return rect.Overlaps(nodeRect);
}

void Impl::linkPopup(bool show, Link link) {
    static constexpr auto name = "Link_Popup";
    if (show) {
        viewData.popupLink = link;
        ImGui::OpenPopup(name);
    }
    if (ImGui::BeginPopup(name)) {
        if (ImGui::MenuItem("Delete Link")) {
            deleteLink(viewData.popupLink);
        }
        ImGui::EndPopup();
    }
}

void Impl::backgroundPopup(bool show) {
    static constexpr auto name = "Background_Popup";
    if (show) {
        ImGui::OpenPopup(name);
    }
    float2 windowPos = ImGui::GetWindowPos();
    if (ImGui::BeginPopup(name)) {
        showDebugger |= ImGui::MenuItem("Show Debugger");
        showStyleEditor |= ImGui::MenuItem("Show Style Editor");
        ImGui::Separator();
        if (delegate) {
            float2 position =
                (float2)ImGui::GetWindowPos() - windowPos - viewData.position;
            delegate->backgroundPopupMenu(position);
        }
        ImGui::EndPopup();
    }
}

void Impl::backgroundBehaviour() {
    bool windowHovered =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow) &&
        ImGui::GetCurrentWindow()->Rect().Contains(ImGui::GetMousePos());
    bool haveNodeInteraction = viewData.activeNode || viewData.activePin;
    bool showPopup = false;
    if (windowHovered && !haveNodeInteraction) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !viewData.isSelectionDragging)
        {
            viewData.isSelectionDragging = true;
            viewData.selectionBegin = ImGui::GetMousePos();
            viewData.selectionEnd = ImGui::GetMousePos();
            selection.beginCandidates();
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            viewData.beginMoveDragging = true;
        }
        showPopup = ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
                    !ImGui::IsMouseDragPastThreshold(ImGuiMouseButton_Right) &&
                    !viewData.isMoveDragging;
    }
    linkPopup(showPopup && viewData.hoveredLink.get(),
              viewData.hoveredLink.get());
    backgroundPopup(showPopup && !viewData.hoveredLink.get());
}

Impl::Impl(CommandPalette& cmd) {
    cmd.add("Delete Selected Nodes", { Key::Backspace, ModFlags::Control },
            [this] { deleteSelected(); });
    cmd.add("Select All", { Key::A, ModFlags::Control },
            [this] { selection.set(model.nodes()); });
}

void NodeEditor::onInput(InputEvent& event) { impl->onInput(event); }

void Impl::onInput(InputEvent& event) {
    using enum InputEventMask;
    event.dispatch<MouseDragged>([&](MouseDragEvent event) {
        if (viewData.beginMoveDragging) {
            viewData.isMoveDragging = true;
            viewData.position += event.offset;
        }
        if (viewData.isSelectionDragging) {
            // FIXME: This must be .locationInWindow when not using viewports
            viewData.selectionEnd = event.locationOnScreen;
            ImRect selectionRect = viewData.getSelectionRect();
            auto candidates = model.nodes() |
                              ranges::views::filter([&](Node const* node) {
                return nodeOverlapsRect(viewData, *node, selectionRect);
            });
            selection.setCandidates(candidates |
                                    ranges::to<utl::hashset<Node*>>);
        }
        return true;
    });
    event.dispatch<ScrollWheel>([&](ScrollEvent event) {
        if (!event.isTrackpad ||
            !viewData.clipRect.Contains(ImGui::GetMousePos()))
        {
            return false;
        }
        viewData.position += event.offset;
        return true;
    });
    event.dispatch<Magnify>([&](MagnificationEvent event) {
        viewData.zoom =
            std::clamp(viewData.zoom + (float)event.offset, 0.5f, 4.0f);
        return true;
    });
}

void NodeEditor::addNode(NodeDesc desc) {
    auto p = std::make_unique<Node>(std::move(desc));
    impl->model.add(std::move(p));
}

NodeEditorGraph const& NodeEditor::model() const { return impl->model; }

/// Computes the position of \p pin relative to its parent node
/// This is the single source of truth of pin positions
static float2 computePinPosition(Pin const& pin, size_t index) {
    // clang-format off
    return visit(pin, utl::overload{
        [&](InputPin const&) {
            return float2(styleVars().PinInset,
                          NodeBodyPadding().y + 
                              styleVars().NodeBodyTitleHeight +
                              index * styleVars().PinAreaSize);
        },
        [&](OutputPin const& pin) {
            return float2(pin.parent().size().x - styleVars().PinInset -
                              styleVars().PinAreaSize,
                          NodeBodyPadding().y +
                              styleVars().NodeBodyTitleHeight +
                              index * styleVars().PinAreaSize);
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
    float2 basePos = (float2)viewData.clipRect.Min + viewData.position;
    return basePos + computeWorldPinPosition(pin);
}

namespace {

struct PinDrawOptions {
    float4 color;
    bool missingInput;
    bool connected;
    bool hovered;
};

} // namespace

static void drawPin(float2 position, PinDrawOptions options) {
    auto* DL = ImGui::GetWindowDrawList();
    float2 center = position + styleVars().PinAreaSize / 2;
    DL->AddCircleFilled(center, styleVars().PinRadius,
                        ImGui::ColorConvertFloat4ToU32(options.color));
    if (options.missingInput) {
        DL->AddCircle(center,
                      styleVars().PinRadius +
                          styleVars().MissingInputIndicatorWidth / 2,
                      ImGui::ColorConvertFloat4ToU32(
                          styleColors().pinMissingIndicator),
                      0, styleVars().MissingInputIndicatorWidth);
    }
    if (options.connected) {
        float radius = ((options.hovered ? styleVars().LinkWidthHovered :
                                           styleVars().LinkWidth) +
                        1) /
                       2;
        DL->AddCircleFilled(center, radius,
                            ImGui::ColorConvertFloat4ToU32(
                                styleColors().pinLinkIndicator));
    }
}

static float2 pinLabelPosition(Pin const& pin, float2 bgPadding,
                               float2 pinPosition, float2 textSize,
                               float ypos) {
    // clang-format off
    return visit(pin, utl::overload{
        [&](InputPin const&) -> float2 {
            return { pinPosition.x - std::max(styleVars().PinInset, 1.0f) - 
                         textSize.x - bgPadding.x,
                     ypos + pinPosition.y };
        },
        [&](OutputPin const&) -> float2 {
            return { pinPosition.x + std::max(styleVars().PinInset, 1.0f) + 
                         styleVars().PinAreaSize + bgPadding.x,
                     ypos + pinPosition.y };
        },
    }); // clang-format on
}

static void displayPinLabelOverlay(Pin const& pin, ViewData const& viewData) {
    float2 pinPosition = computeScreenPinPosition(viewData, pin);
    auto* DL = ImGui::GetWindowDrawList();
    auto* textBegin = pin.name().data();
    auto* textEnd = textBegin + pin.name().size();
    float2 textSize = ImGui::CalcTextSize(textBegin, textEnd);
    float ypos = styleVars().PinAreaSize / 2 - textSize.y / 2;
    float2 bgPadding = 3;
    float bgRounding = 3;
    auto bgCol = ImGui::GetColorU32(ImGuiCol_PopupBg);
    float2 textPos =
        pinLabelPosition(pin, bgPadding, pinPosition, textSize, ypos);
    DL->AddRectFilled(textPos - bgPadding, textPos + textSize + bgPadding,
                      bgCol, bgRounding);
    DL->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), textBegin, textEnd);
}

static bool isCompatible(Pin const& a, Pin const& b) {
    return &a.parent() != &b.parent() && a.type() != b.type();
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

static LinkDrawData computeLinkDrawData(float2 begin, float2 end) {
    LinkDrawData dd{};
    dd.begin = begin;
    dd.end = end;
    dd.mid = (begin + end) / 2;
    dd.area = vml::abs(end - begin);
    float crtlPointFactor =
        2.0f *
        std::atan(dd.area.y / styleVars().LinkBezierControlPointYFalloff) /
        vml::constants<float>::pi;
    float ctrlPointX =
        crtlPointFactor * std::clamp(dd.area.x,
                                     styleVars().LinkBezierControlPointXMin,
                                     styleVars().LinkBezierControlPointXMax);
    dd.beginControlPoint = dd.begin + float2(ctrlPointX, 0);
    dd.endControlPoint = dd.end - float2(ctrlPointX, 0);
    return dd;
}

static bool testPathOverlap(std::span<ImVec2 const> path, float2 point) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
        float2 p1 = path[i];
        float2 p2 = path[i + 1];
        if (vml::distance(vml::line_segment_2D<>(p1, p2), point) <=
            styleVars().LinkHoverDist)
        {
            return true;
        }
    }
    return false;
}

static bool testLinkOverlap(float2 begin, float2 end,
                            std::span<ImVec2 const> path, float2 point) {
    vml::AABB<float, 2> rect(begin, end);
    vml::AABB<float, 2> wideRect(rect.lower_bound() -
                                     float2(styleVars().LinkHoverDist),
                                 rect.upper_bound() +
                                     float2(styleVars().LinkHoverDist));
    if (!vml::do_intersect(wideRect, point)) {
        return false;
    }
    return testPathOverlap(path, point);
}

/// Draws a link from \p begin to \p end in screen coordinates
/// \Returns `true` if \p mousePos overlaps the link
static bool displayLink(float2 begin, float2 end,
                        std::optional<float2> mousePos, bool mayHighlight,
                        bool mustHighlight) {
    auto* DL = ImGui::GetWindowDrawList();
    auto dd = computeLinkDrawData(begin, end);
    DL->PathLineTo(begin);
    DL->PathBezierQuadraticCurveTo(dd.beginControlPoint, dd.mid, 0);
    DL->PathBezierQuadraticCurveTo(dd.endControlPoint, end, 0);
    auto col = ImGui::ColorConvertFloat4ToU32(styleColors().link);
    bool hovered = mousePos &&
                   testLinkOverlap(begin, end, DL->_Path, *mousePos);
    bool drawHighlighted = (mayHighlight && hovered) || mustHighlight;
    float width = drawHighlighted ? styleVars().LinkWidthHovered :
                                    styleVars().LinkWidth;
    DL->PathStroke(col, ImDrawFlags_None, width);
    return hovered;
}

static void drawLink(float2 begin, float2 end, bool highlighted) {
    displayLink(begin, end, std::nullopt, /* mayHighlight: */ true,
                /* mustHighlight: */ highlighted);
}

static void displayPinLink(ViewData& viewData, InputPin& pin,
                           float2 pinPosition) {
    auto* origin = pin.origin();
    if (!origin) {
        return;
    }
    Link link = { origin, &pin };
    float2 begin = computeScreenPinPosition(viewData, *origin) +
                   styleVars().PinAreaSize / 2;
    float2 end = pinPosition + styleVars().PinAreaSize / 2;
    bool mayHighlight = viewData.hoveredLink.getLast() == link;
    bool mustHighlight = link.contains(viewData.hoveredPin.getLast()) &&
                         !viewData.activePin;
    bool hovered = displayLink(begin, end, ImGui::GetMousePos(), mayHighlight,
                               mustHighlight);
    if (hovered && !viewData.activePin) {
        viewData.hoveredLink.set(link);
    }
}

void Impl::displayLinks(Node& node) {
    float2 nodepos =
        (float2)ImGui::GetWindowPos() + node.position() + viewData.position;
    ImGui::PushID("Inputs");
    for (auto [index, pin]: node.inputs() | enumerate) {
        displayPinLink(viewData, *pin,
                       nodepos + computePinPosition(*pin, index));
    }
    ImGui::PopID();
    // Display dragging link
    if (viewData.activePin && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        float2 pos = computeScreenPinPosition(viewData, *viewData.activePin) +
                     styleVars().PinAreaSize / 2;
        bool highlighted = true;
        if (isa<InputPin>(viewData.activePin)) {
            drawLink(ImGui::GetMousePos(), pos, highlighted);
        }
        else {
            drawLink(pos, ImGui::GetMousePos(), highlighted);
        }
    }
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
        pin.clearLinks();
    }
    ImGui::EndPopup();
}

namespace {

enum class TextAlign { Left, Right };

} // namespace

static void displayPinLabelInBody(std::string_view label, float2 position,
                                  TextAlign align, float& maxLabelWidth) {
    char const* begin = label.data();
    char const* end = begin + label.size();
    float2 textSize = ImGui::CalcTextSize(begin, end);
    position.y -= textSize.y / 2;
    if (align == TextAlign::Right) {
        position.x -= textSize.x;
    }
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddText(position,
                ImGui::ColorConvertFloat4ToU32(styleColors().nodeBodyText),
                begin, end);
    maxLabelWidth = std::max(maxLabelWidth, textSize.x);
}

static bool isOptional(Pin const& pin) {
    return test(pin.desc().flags & PinFlags::Optional);
}

static float4 selectPinColor(Pin const& pin) {
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

static float2 pinInBodyLabelPosition(Pin const& pin, float2 pinPosition) {
    // clang-format off
    return pinPosition + visit(pin, utl::overload{
        [&](InputPin const&) {
            return float2(styleVars().PinAreaSize, styleVars().PinAreaSize / 2);
        },
        [&](OutputPin const&) { return float2(0, styleVars().PinAreaSize / 2) ; },
    }); // clang-format on
}

void Impl::displayPin(Pin& pin, float2 position, float& maxLabelWith,
                      bool displayLabelInBody) {
    ImGui::PushID(&pin);
    ImRect bb(position, position + float2(styleVars().PinAreaSize));
    ImGui::ItemSize(float2(styleVars().PinAreaSize));
    auto ID = ImGui::GetID("Pin_Handle");
    ImGui::ItemAdd(bb, ID);
    bool hovered = ImGui::ItemHoverable(bb, ID, ImGuiItemFlags_AllowOverlap);
    bool clickedLeft = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool clickedRight = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (clickedLeft || clickedRight) {
        viewData.activePin = &pin;
        if (ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 2) {
            pin.clearLinks();
        }
    }
    pinPopup(pin, /* show: */ clickedRight);
    bool isDraggingCompatiblePin = viewData.activePin &&
                                   isCompatible(*viewData.activePin, pin);
    bool drawConnected = isConnected(pin) ||
                         (hovered && isDraggingCompatiblePin) ||
                         (viewData.activePin == &pin &&
                          ImGui::IsMouseDown(ImGuiMouseButton_Left));
    bool drawHovered =
        (hovered && (!viewData.activePin || isDraggingCompatiblePin)) ||
        viewData.hoveredLink.get().contains(&pin) ||
        (!viewData.activePin && viewData.hoveredPin.getLast() &&
         isLinked(pin, *viewData.hoveredPin.getLast())) ||
        viewData.activePin == &pin;
    drawPin(position, { .color = selectPinColor(pin),
                        .missingInput = inputMissing(pin),
                        .connected = drawConnected,
                        .hovered = drawHovered });
    if (displayLabelInBody) {
        displayPinLabelInBody(pin.name(), pinInBodyLabelPosition(pin, position),
                              isa<InputPin>(pin) ? TextAlign::Left :
                                                   TextAlign::Right,
                              maxLabelWith);
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

static float2 computeNodeMinSize(Node const& node, float2 bodyPadding) {
    float pinHeight = styleVars().PinAreaSize *
                      std::max(node.inputs().size(), node.outputs().size());
    return vml::max(node.layoutData().minSize,
                    float2(0, pinHeight + styleVars().NodeBodyTitleHeight)) +
           2 * bodyPadding + float2(0, styleVars().NodeBodyBottomPadding);
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
    float2 nodepos = (float2)ImGui::GetWindowPos() + NodeOuterPadding();
    float maxInputLabelWidth = 0;
    float maxOutputLabelWidth = 0;
    {
        ImGui::PushID("Inputs");
        bool displayInBody = alwaysDisplayInputNames(node);
        for (auto [index, pin]: node.inputs() | enumerate) {
            displayPin(*pin, nodepos + computePinPosition(*pin, index),
                       maxInputLabelWidth, displayInBody);
        }
        ImGui::PopID();
    }
    {
        ImGui::PushID("Outputs");
        bool displayInBody = alwaysDisplayOutputNames(node);
        for (auto [index, pin]: node.outputs() | enumerate) {
            displayPin(*pin, nodepos + computePinPosition(*pin, index),
                       maxOutputLabelWidth, displayInBody);
        }
        ImGui::PopID();
    }
    node.layoutData().minSize.x = maxInputLabelWidth + maxOutputLabelWidth;
}

/// \Returns the area in which content can be drawn
static ImRect drawNodeBody(Node const& node, bool selected) {
    float2 pos = (float2)ImGui::GetWindowPos() + NodeOuterPadding();
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(pos, pos + node.size(),
                      ImGui::ColorConvertFloat4ToU32(node.color()),
                      styleVars().NodeBodyRounding + 1);
    if (selected) {
        DL->AddRect(pos + styleVars().NodeBodySelectionOutlineInset,
                    pos + node.size() -
                        styleVars().NodeBodySelectionOutlineInset,
                    ImGui::ColorConvertFloat4ToU32(styleColors().whiteOutline),
                    styleVars().NodeBodyRounding -
                        styleVars().NodeBodySelectionOutlineInset,
                    ImDrawFlags_None,
                    2 * styleVars().NodeBodySelectionOutlineInset);
    }
    float highlightOutlineInset = 1;
    DL->AddRect(pos + highlightOutlineInset,
                pos + node.size() - highlightOutlineInset,
                ImGui::ColorConvertFloat4ToU32(styleColors().whiteOutline),
                styleVars().NodeBodyRounding - highlightOutlineInset);
    DL->AddRect(pos, pos + node.size(),
                ImGui::ColorConvertFloat4ToU32(styleColors().blackOutline),
                styleVars().NodeBodyRounding);
    auto* font =
        FontManager::get(FontDesc::UIDefault().setWeight(FontWeight::Bold));
    ImGui::PushFont(font);
    auto* label = node.name().data();
    auto* labelEnd = node.name().data() + node.name().size();
    float2 labelSize =
        vml::min((float2)ImGui::CalcTextSize(label, labelEnd),
                 node.size() -
                     float2(2 * (styleVars().NodeBodySelectionOutlineInset + 1),
                            0));
    float2 labelPos =
        pos + float2((node.size().x - labelSize.x) / 2, NodeBodyPadding().y);
    auto labelCol = ImGui::ColorConvertFloat4ToU32(styleColors().nodeBodyText);
    ImGui::PushClipRect(labelPos, labelPos + labelSize, true);
    DL->AddText(labelPos, labelCol, label, labelEnd);
    ImGui::PopClipRect();
    ImGui::PopFont();
    float2 contentRectMin =
        pos + NodeBodyPadding() + float2(0, labelSize.y + NodeBodyPadding().y);
    float2 contentRectMax = pos + node.size() - NodeBodyPadding();
    return ImRect(contentRectMin, contentRectMax);
}

/// \Returns true if the background has been clicked with the left or right
/// mouse button
static bool displayNodeContent(ImRect contentRect, Node const& node) {
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
    }
    ImGui::EndChild();
    return bgPressed;
}

static void displayNodeResizeGrip(ViewData& viewData, Node& node) {
    float2 areaSize = { 10, 10 };
    float2 pos = (float2)ImGui::GetWindowPos() + NodeOuterPadding() +
                 node.size() - areaSize;
    ImGuiID id = ImGui::GetID("Node_Resize_Grip");
    ImRect bb(pos, pos + areaSize);
    ImGui::ItemSize(areaSize);
    ImGui::ItemAdd(bb, id);
    bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
    if (ImGui::IsItemClicked()) {
        viewData.activeResize = &node;
        viewData.resizeSize = node.size();
    }
    bool dragging = viewData.activeResize == &node &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragging) {
        float2 minSize = computeNodeMinSize(node, NodeBodyPadding());
        viewData.resizeSize += ImGui::GetIO().MouseDelta;
        float2 newSize = vml::max(viewData.resizeSize, minSize);
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

void Impl::displayNodeBody(Node& node) {
    float2 pos =
        (float2)ImGui::GetWindowPos() + viewData.position + node.position();
    float2 windowPos = pos - NodeOuterPadding();
    float2 windowSize = node.size() + 2 * NodeOuterPadding();
    bool open = beginInvisibleWindow("Body", windowPos, windowSize);
    if (!open && viewData.activeNode != &node) {
        ImGui::EndChild();
        return;
    }
    auto* DL = ImGui::GetWindowDrawList();
    DL->PushClipRect(viewData.clipRect.Min, viewData.clipRect.Max);
    bool isSelected = selection.contains(&node);
    ImRect contentRect = drawNodeBody(node, isSelected);
    ImGuiID id = ImGui::GetID("Node_Body");
    ImRect bb(pos, pos + node.size());
    ImGui::ItemSize(node.size());
    ImGui::ItemAdd(bb, id);
    if (ImGui::IsMouseHoveringRect(windowPos + 2, windowPos + windowSize - 4)) {
        viewData.hoveredLink.set({});
        viewData.hoveredNode.set(&node);
    }
    bool bgClickedLeft = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    bool bgClickedRight = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    displayPins(node);
    if (displayNodeContent(contentRect, node)) {
        bgClickedLeft |= ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bgClickedRight |= ImGui::IsMouseDown(ImGuiMouseButton_Right);
    }
    displayNodeResizeGrip(viewData, node);
    if (bgClickedLeft || bgClickedRight) {
        orderFront(&node);
        viewData.activeNode = &node;
    }
    /// Selected nodes change selection state when mouse is released
    if (isSelected && viewData.activeNode == &node &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !viewData.isDraggingNode && !viewData.handledSelectionSinceMousePressed)
    {
        selection.amend(&node);
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
        selection.amend(&node);
    }
    nodeBodyPopup(node,
                  /* show: */ bgClickedRight && !haveSecondaryInteraction);
    bool dragging = [&] {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            return false;
        }
        return !haveSecondaryInteraction;
    }();
    if (dragging) {
        viewData.isDraggingNode = true;
        float2 delta = ImGui::GetIO().MouseDelta;
        for (auto* node: selection.nodes()) {
            node->setPosition(node->position() + delta);
        }
    }
    DL->PopClipRect();
    ImGui::EndChild();
}

void Impl::displayNode(Node& node) {
    node.setSize(
        vml::max(node.size(), computeNodeMinSize(node, NodeBodyPadding())));
    ImGui::PushID(&node);
    displayNodeBody(node);
    ImGui::PopID();
}

void NodeEditor::display() { impl->display(); }

void Impl::display() {
    auto flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs;
    if (!ImGui::BeginChild("Node_Editor", {}, ImGuiChildFlags_None, flags)) {
        ImGui::EndChild();
        return;
    }
    viewData.beginFrame();
    drawBackground(viewData.position);
    /// We make a copy here because we may reorder the nodes during traversal.
    /// This may seem wasteful but I'm sure it's fine
    auto nodes = model.nodes() | ranges::to<std::vector>;
    for (auto* node: nodes) {
        displayLinks(*node);
    }
    for (auto* node: nodes) {
        displayNode(*node);
    }
    backgroundBehaviour();
    drawOverlays();
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
        viewDataDebugger(viewData, &showDebugger);
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
    return isCompatible(hoveredPin, *viewData.activePin);
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

void Impl::orderFront(Node* node) { model.orderFront(node); }

void Impl::deleteSelected() {
    auto pred = [&](Node const& node) { return selection.contains(&node); };
    model.deleteIf(pred);
}

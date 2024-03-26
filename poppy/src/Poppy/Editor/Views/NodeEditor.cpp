#include "Poppy/Editor/Views/NodeEditor.h"

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/overload.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Debug.h"
#include "Poppy/UI/Font.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

using Impl = NodeEditor::Impl;

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
        last->removeTarget(this);
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
    auto itr = ranges::find(_targets, pin);
    if (itr != _targets.end()) {
        auto* target = *itr;
        _targets.erase(itr);
        target->setOrigin(nullptr);
    }
}

void OutputPin::clearTargets() {
    for (auto* target: targets()) {
        target->setOrigin(nullptr);
    }
    _targets.clear();
}

static bool findCycleDFS(Node const* node, utl::hashset<Node const*>& visited) {
    return ranges::any_of(node->successors(), [&](Node const* succ) {
        return !visited.insert(succ).second || findCycleDFS(succ, visited);
    });
}

static bool findCycle(Node const* node) {
    utl::hashset<Node const*> visited;
    return findCycleDFS(node, visited);
}

static void link(OutputPin& out, InputPin& in) {
    BL_ASSERT(!findCycle(&out.parent()));
    out.addTarget(&in);
    in.setOrigin(&out);
    if (findCycle(&out.parent())) {
        Logger::Error("Link would introduce a cycle. Not linking");
        out.removeTarget(&in);
        BL_ASSERT(!in.origin());
    }
}

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

static auto succsImpl(auto const& outputs) {
    return outputs |
           ranges::views::transform([](auto* pin) { return pin->targets(); }) |
           ranges::views::join |
           ranges::views::transform([](auto* pin) { return &pin->parent(); });
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

bool Graph::hasCycles() const {
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

utl::small_vector<Node*> Graph::sources() {
    return sourcesImpl(nodes()) | ranges::to<utl::small_vector<Node*>>;
}

utl::small_vector<Node const*> Graph::sources() const {
    return sourcesImpl(nodes()) | ranges::to<utl::small_vector<Node const*>>;
}

static auto sinksImpl(auto const& nodes) {
    return nodes | filter([](Node const* node) {
        return ranges::all_of(node->outputs(), [](OutputPin const* pin) {
            return pin->targets().empty();
        });
    });
}

utl::small_vector<Node*> Graph::sinks() {
    return sinksImpl(nodes()) | ranges::to<utl::small_vector<Node*>>;
}

utl::small_vector<Node const*> Graph::sinks() const {
    return sinksImpl(nodes()) | ranges::to<utl::small_vector<Node const*>>;
}

namespace {

enum class Axis { Horizontal, Vertical };

struct PersistentViewData {
    vml::float2 position = { 0.0, 0.0 };
    float zoom = 1.0;
};

struct ViewData: PersistentViewData {
    bool isBGHovered = false;
    bool isDraggingNode = false;
    Node* hoveredNode = nullptr;
    Node* activeNode = nullptr;
    Node* activeResize = nullptr;
    Pin* activePin = nullptr;
};

struct Selection {
    std::vector<size_t> indices;
};

} // namespace

Node::Node(NodeDesc desc): _desc(std::move(desc)) {
    _inputs = _desc.inputs | transform([&](PinDesc const& pin) {
        return UniquePtr<InputPin>(new InputPin(pin, *this));
    }) | ranges::to<std::vector>;
    _outputs = _desc.outputs | transform([&](PinDesc const& pin) {
        return UniquePtr<OutputPin>(new OutputPin(pin, *this));
    }) | ranges::to<std::vector>;
}

struct NodeEditor::Impl {
    void display();

    void onInput(bloom::InputEvent& event);

    std::vector<Node*> currentNodeOrder;
    Graph graph;
    Selection selection;
    ViewData viewData;
};

NodeEditor::NodeEditor(): impl(std::make_unique<Impl>()) {}

NodeEditor::~NodeEditor() = default;

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

static void drawBackground(float2 viewPos, float zoom) {
    float dist = 10;
    dist *= zoom;
    float4 color = ImGui::GetStyleColorVec4(ImGuiCol_Separator);
    float2 begin = vml::fmod(viewPos, dist);
    drawLines(begin.x, ImGui::GetWindowWidth(), dist, Axis::Horizontal, color);
    drawLines(begin.y, ImGui::GetWindowHeight(), dist, Axis::Vertical, color);
    ImGui::Text("Position: %f, %f", viewPos.x, viewPos.y);
    ImGui::Text("Zoom: %f", zoom);
}

static void backgroundBehaviour(ViewData& viewData) {
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InvisibleButton("Node_Editor_Background_Button",
                               ImGui::GetContentRegionAvail(),
                               ImGuiButtonFlags_MouseButtonRight))
    {
        // Clear selection
    }
    viewData.isBGHovered = ImGui::IsItemHovered();
}

void NodeEditor::onInput(InputEvent& event) { impl->onInput(event); }

void Impl::onInput(InputEvent& event) {
    using enum InputEventMask;
    event.dispatch<RightMouseDragged>([&](MouseDragEvent event) {
        viewData.position += event.offset;
        return true;
    });
    event.dispatch<ScrollWheel>([&](ScrollEvent event) {
        if (!event.isTrackpad) {
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
    impl->currentNodeOrder.push_back(p.get());
    impl->graph.add(std::move(p));
}

Graph const& NodeEditor::graph() const { return impl->graph; }

static float4 const WhiteOutlineColor = { 1, 1, 1, 0.5 };
static float4 const BlackOutlineColor = { 0, 0, 0, 0.7 };
static float4 const LinkColor = { 1, 1, 1, 0.8 };
static float4 PinColor() { return ImGui::GetStyleColorVec4(ImGuiCol_PopupBg); }
static float2 const MinNodeSize = { 80, 40 };
static float2 const NodeBodyPadding = { 5, 5 };
static float const PinSize = 20;

/// Computes the position of \p pin relative to its parent node
static float2 computePinPosition(Pin const& pin, size_t index) {
    // clang-format off
    return visit(pin, utl::overload{
        [&](InputPin const&) {
            return float2(PinSize * -0.4, NodeBodyPadding.y) +
                float2(0, PinSize * index);
        },
        [&](OutputPin const& pin) {
            return float2(pin.parent().size().x + PinSize * -0.6,
                          NodeBodyPadding.y) +
                float2(0, PinSize * index);
        }
    }); // clang-format on
}

/// Computes the position of \p pin relative to the view
static float2 computeAbsPinPosition(Pin const& pin) {
    return pin.parent().position() +
           computePinPosition(pin, pin.indexInParent());
}

static void drawPin(float2 position) {
    auto* DL = ImGui::GetWindowDrawList();
    float radius = 6;
    DL->AddCircleFilled(position + PinSize / 2, radius,
                        ImGui::ColorConvertFloat4ToU32(PinColor()));
    DL->AddCircle(position + PinSize / 2, radius,
                  ImGui::ColorConvertFloat4ToU32(WhiteOutlineColor));
}

static void displayPinLabel(Pin const& pin, float2 pinPosition) {
    auto* DL = ImGui::GetWindowDrawList();
    auto* textBegin = pin.name().data();
    auto* textEnd = textBegin + pin.name().size();
    float2 textSize = ImGui::CalcTextSize(textBegin, textEnd);
    float ypos = PinSize / 2 - textSize.y / 2;
    float2 bgPadding = 3;
    float bgRounding = 3;
    auto bgCol = ImGui::GetColorU32(ImGuiCol_PopupBg);
    float2 textPos = isa<InputPin>(pin) ?
                         pinPosition + float2(-textSize.x, ypos) :
                         pinPosition + float2(PinSize, ypos);
    DL->AddRectFilled(textPos - bgPadding, textPos + textSize + bgPadding,
                      bgCol, bgRounding);
    DL->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), textBegin, textEnd);
}

static bool isCompatible(Pin const& a, Pin const& b) {
    return &a.parent() != &b.parent() && a.type() != b.type();
}

namespace {

struct LinkDrawData {
    float2 begin;
    float2 end;
    float2 mid;
    float2 area;
    float2 beginControlPoint;
    float2 endControlPoint;
};

} // namespace

static LinkDrawData getLinkDrawData(float2 begin, float2 end) {
    LinkDrawData dd{};
    dd.begin = begin;
    dd.end = end;
    dd.mid = (begin + end) / 2;
    dd.area = vml::abs(end - begin);
    float crtlPointFactor =
        2.0f * std::atan(dd.area.y / 100) / vml::constants<float>::pi;
    float ctrlPointX = crtlPointFactor * std::clamp(dd.area.x, 100.f, 200.0f);
    dd.beginControlPoint = dd.begin + float2(ctrlPointX, 0);
    dd.endControlPoint = dd.end - float2(ctrlPointX, 0);
    return dd;
}

static void strokeLink(ImDrawList* DL) {
    auto col = ImGui::ColorConvertFloat4ToU32(LinkColor);
    DL->PathStroke(col, ImDrawFlags_None, 2);
}

static void drawLinkBegin(float2 begin, float2 end) {
    auto* DL = ImGui::GetWindowDrawList();
    auto dd = getLinkDrawData(begin, end);
    DL->PathLineTo(begin);
    DL->PathBezierQuadraticCurveTo(dd.beginControlPoint, dd.mid, 0);
    strokeLink(DL);
}

static void drawLinkEnd(float2 begin, float2 end) {
    auto* DL = ImGui::GetWindowDrawList();
    auto dd = getLinkDrawData(begin, end);
    DL->PathLineTo(dd.mid);
    DL->PathBezierQuadraticCurveTo(dd.endControlPoint, end, 0);
    strokeLink(DL);
}

static void drawLink(float2 begin, float2 end) {
    auto* DL = ImGui::GetWindowDrawList();
    auto dd = getLinkDrawData(begin, end);
    DL->PathLineTo(begin);
    DL->PathBezierQuadraticCurveTo(dd.beginControlPoint, dd.mid, 0);
    DL->PathBezierQuadraticCurveTo(dd.endControlPoint, end, 0);
    strokeLink(DL);
}

static void drawPinLinks(ViewData const& viewData, Pin const& pin,
                         float2 pinPosition) {
    float2 basePos = (float2)ImGui::GetWindowPos() + viewData.position;
    // clang-format off
    visit(pin, utl::overload{
        [&](InputPin const& pin) {
            if (auto* origin = pin.origin()) {
                float2 begin = basePos + computeAbsPinPosition(*origin) +
                               PinSize / 2;
                drawLinkEnd(begin, pinPosition + PinSize / 2);
            }
        },
        [&](OutputPin const& pin) {
            for (auto* target: pin.targets()) {
                float2 end = basePos + computeAbsPinPosition(*target) +
                             PinSize / 2;
                drawLinkBegin(pinPosition + PinSize / 2, end);
            }
        }
    }); // clang-format on
}

static void displayPin(ViewData& viewData, Pin& pin, float2 position) {
    ImGui::PushID(&pin);
    ImRect bb(position, position + float2(PinSize));
    ImGui::ItemSize(float2(PinSize));
    auto ID = ImGui::GetID("Pin_Handle");
    ImGui::ItemAdd(bb, ID);
    bool hovered = ImGui::ItemHoverable(bb, ID, ImGuiItemFlags_AllowOverlap);
    if (ImGui::IsItemClicked()) {
        viewData.activePin = &pin;
        if (ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 2) {
            pin.clearLinks();
        }
    }
    bool dragging = viewData.activePin == &pin &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragging) {
        if (isa<InputPin>(pin)) {
            drawLink(ImGui::GetMousePos(), bb.GetCenter());
        }
        else {
            drawLink(bb.GetCenter(), ImGui::GetMousePos());
        }
    }
    drawPinLinks(viewData, pin, position);
    drawPin(position);
    bool isDraggingCompatiblePin = viewData.activePin &&
                                   isCompatible(*viewData.activePin, pin);
    if (hovered && !dragging &&
        (!viewData.activePin || isDraggingCompatiblePin))
    {
        displayPinLabel(pin, position);
    }
    if (hovered && isDraggingCompatiblePin &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        link(*viewData.activePin, pin);
    }
    ImGui::PopID();
}

static float2 computeMinNodeSize(Node const& node, float2 bodyPadding) {
    float2 inner = { MinNodeSize.x,
                     std::max(MinNodeSize.y,
                              PinSize * std::max(node.inputs().size(),
                                                 node.outputs().size())) };
    return inner + 2 * bodyPadding;
}

static void displayInputsOutputs(ViewData& viewData, Node& node) {
    float2 winpos = ImGui::GetWindowPos();
    float2 nodepos = winpos + viewData.position + node.position();
    ImGui::PushID("Inputs");
    for (auto [index, pin]: node.inputs() | enumerate) {
        displayPin(viewData, *pin, nodepos + computePinPosition(*pin, index));
    }
    ImGui::PopID();
    ImGui::PushID("Outputs");
    for (auto [index, pin]: node.outputs() | enumerate) {
        displayPin(viewData, *pin, nodepos + computePinPosition(*pin, index));
    }
    ImGui::PopID();
}

static void drawNodeBody(ViewData const& viewData, Node const& node) {
    float2 winpos = ImGui::GetWindowPos();
    float2 pos = winpos + viewData.position + node.position();
    float rounding = 5;
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(pos, pos + node.size(),
                      ImGui::ColorConvertFloat4ToU32(node.desc().color),
                      rounding + 1);
    DL->AddRect(pos + float2(1, 1), pos + node.size() - float2(2, 2),
                ImGui::ColorConvertFloat4ToU32(WhiteOutlineColor),
                rounding - 1);
    DL->AddRect(pos, pos + node.size(),
                ImGui::ColorConvertFloat4ToU32(BlackOutlineColor), rounding);
    auto* font =
        FontManager::get(FontDesc::UIDefault().setWeight(FontWeight::Bold));
    ImGui::PushFont(font);
    auto* label = node.name().data();
    auto* labelEnd = node.name().data() + node.name().size();
    float2 labelSize =
        vml::min((float2)ImGui::CalcTextSize(label, labelEnd),
                 node.size() - float2(PinSize + 2 * NodeBodyPadding.x, 0));
    float2 labelPos = pos + float2((node.size().x - labelSize.x) / 2, rounding);
    auto labelCol =
        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text));
    ImGui::PushClipRect(labelPos, labelPos + labelSize, true);
    DL->AddText(labelPos, labelCol, label, labelEnd);
    ImGui::PopClipRect();
    ImGui::PopFont();
}

static void displayNodeBody(ViewData& viewData, Node& node) {
    drawNodeBody(viewData, node);
    float2 winpos = ImGui::GetWindowPos();
    float2 pos = winpos + viewData.position + node.position();
    ImGuiID id = ImGui::GetID("Node_Body");
    ImRect bb(pos, pos + node.size());
    ImGui::ItemSize(node.size());
    ImGui::ItemAdd(bb, id);
    bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
    if (hovered) {
        viewData.hoveredNode = &node;
    }
    if (ImGui::IsItemClicked()) {
        viewData.activeNode = &node;
    }
    bool dragging = [&] {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) return false;
        return viewData.activeNode == &node && !viewData.activeResize &&
               !viewData.activePin;
    }();
    if (dragging) {
        node.setPosition(node.position() + (float2)ImGui::GetIO().MouseDelta);
    }
}

static void displayNodeResizeGrip(ViewData& viewData, Node& node,
                                  float2 minSize) {
    float2 areaSize = { 10, 10 };
    float2 winpos = ImGui::GetWindowPos();
    float2 pos =
        winpos + viewData.position + node.position() + node.size() - areaSize;
    ImGuiID id = ImGui::GetID("Node_Resize_Grip");
    ImRect bb(pos, pos + areaSize);
    ImGui::ItemSize(areaSize);
    ImGui::ItemAdd(bb, id);
    bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
    if (ImGui::IsItemClicked()) {
        viewData.activeResize = &node;
    }
    bool dragging = viewData.activeResize == &node &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragging) {
        node.setSize(
            vml::max(node.size() + (float2)ImGui::GetIO().MouseDelta, minSize));
    }
    if (hovered || dragging) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
    }
}

static void displayNode(ViewData& viewData, Node& node) {
    float2 minSize = computeMinNodeSize(node, NodeBodyPadding);
    node.setSize(vml::max(node.size(), minSize));
    ImGui::PushID(&node);
    displayNodeBody(viewData, node);
    displayNodeResizeGrip(viewData, node, minSize);
    displayInputsOutputs(viewData, node);
    ImGui::PopID();
}

void NodeEditor::display() { impl->display(); }

void Impl::display() {
    if (!ImGui::BeginChild("Node_Editor", {}, ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar))
    {
        return;
    }
    drawBackground(viewData.position, viewData.zoom);
    backgroundBehaviour(viewData);
    viewData.hoveredNode = nullptr;
    for (auto* node: currentNodeOrder) {
        displayNode(viewData, *node);
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        viewData.activeNode = nullptr;
        viewData.activeResize = nullptr;
        viewData.activePin = nullptr;
    }
    ImGui::EndChild();
}

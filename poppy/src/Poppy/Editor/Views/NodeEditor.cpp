#include "Poppy/Editor/Views/NodeEditor.h"

#include <optional>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/overload.hpp>
#include <utl/stack.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Debug.h"
#include "Poppy/UI/Font.h"

using namespace poppy;
using namespace nodeEditor;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

/// # Style

static float4 const WhiteOutlineColor = { 1, 1, 1, 0.5 };
static float4 const BlackOutlineColor = { 0, 0, 0, 0.7 };
static float4 const LinkColor = { 1, 1, 1, 0.8 };
static float4 const PinColorConnected = { 0.2, 0.2, 0.2, 1 };
static float4 const PinColorOptional = { 0.4, 0.4, 0.4, 1 };
static float4 const PinColorMissing = { 1, 0, 0.2, 1 };
static float const PinSize = 20;
static float2 const NodeBodyPadding = { 5, 5 };
static float2 const NodeOuterPadding = { PinSize * 0.6, 1 };
static float4 const SelectionRectColor = { 1, 1, 1, 0.3 };
static float2 const BackgroundLineDist = { 30, 30 };

using Impl = NodeEditor::Impl;

static ImRect toRect(float2 begin, float2 end) {
    return ImRect(vml::min(begin, end), vml::max(begin, end));
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

/// Data that persists between frames when performing some actions
struct ActiveViewData {
    /// True if a node is currently being dragged
    bool isDraggingNode = false;
    /// True if we already changed selection state since the mouse was pressed
    bool handledSelectionSinceMousePresed = false;
    /// True if we are selecting be dragged the background
    bool isSelectionDragging = false;
    bool isMoveDragging = false;
    Node* activeNode = nullptr;
    Node* activeResize = nullptr;
    Pin* activePin = nullptr;
    float2 resizeSize = 0;
    float2 selectionBegin = 0;
    float2 selectionEnd = 0;

    ImRect getSelectionRect() const {
        return toRect(selectionBegin, selectionEnd);
    }
};

struct ViewData: PersistentViewData, ActiveViewData {
    ImRect clipRect = {};

    void resetActiveData() {
        static_cast<ActiveViewData&>(*this) = ActiveViewData{};
    }
};

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

    ///
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
    void set(Node* node) {
        clear();
        add(node);
    }

    /// Unselectes all nodes
    void clear() { _nodes.clear(); }

    /// \Returns a view over all selected nodes
    std::span<Node* const> nodes() const { return _nodes.values(); }

    ///
    void beginCandidates() { this->mode = getSelectionMode(); }

    ///
    void setCandidates(utl::hashset<Node*> nodes) {
        _candidates = std::move(nodes);
    }

    ///
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

Node::Node(NodeDesc desc): _desc(std::move(desc)) {
    _inputs = _desc.inputs | transform([&](PinDesc const& pin) {
        return UniquePtr<InputPin>(new InputPin(pin, *this));
    }) | ranges::to<std::vector>;
    _outputs = _desc.outputs | transform([&](PinDesc const& pin) {
        return UniquePtr<OutputPin>(new OutputPin(pin, *this));
    }) | ranges::to<std::vector>;
}

struct NodeEditor::Impl {
    void onInput(bloom::InputEvent& event);

    void display();

    void backgroundBehaviour();
    void drawOverlays();
    void displayNodeBody(Node& node);
    void displayNode(Node& node);

    void orderFront(Node* node);

    std::vector<Node*> currentNodeOrder;
    Graph graph;
    Selection selection;
    ViewData viewData;
};

NodeEditor::NodeEditor(): impl(std::make_unique<Impl>()) {}

NodeEditor::~NodeEditor() = default;

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
    float2 begin = vml::fmod(viewPos, BackgroundLineDist);
    drawLines(begin.x, ImGui::GetWindowWidth(), BackgroundLineDist.x,
              Axis::Horizontal, color);
    drawLines(begin.y, ImGui::GetWindowHeight(), BackgroundLineDist.y,
              Axis::Vertical, color);
}

void Impl::backgroundBehaviour() {
    if (!viewData.clipRect.Contains(ImGui::GetMousePos()) ||
        viewData.activeNode || viewData.activePin)
    {
        return;
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (!viewData.isSelectionDragging) {
            viewData.isSelectionDragging = true;
            viewData.selectionBegin = ImGui::GetMousePos();
            selection.beginCandidates();
        }
        viewData.selectionEnd = ImGui::GetMousePos();
        ImRect selectionRect = viewData.getSelectionRect();
        auto candidates = graph.nodes() |
                          ranges::views::filter([&](Node const* node) {
            auto rect = toRect(*node);
            rect.Translate((float2)viewData.clipRect.Min + viewData.position);
            Logger::Trace("Node Rect Min: ", float2(rect.Min));
            return selectionRect.Overlaps(rect);
        });
        selection.setCandidates(candidates | ranges::to<utl::hashset<Node*>>);
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        viewData.isMoveDragging = true;
    }
}

void Impl::drawOverlays() {
    if (!beginInvisibleWindow("Overlays", ImGui::GetWindowPos(),
                              ImGui::GetWindowSize(),
                              ImGuiWindowFlags_NoInputs))
    {
        ImGui::EndChild();
        return;
    }
    if (viewData.isSelectionDragging) {
        Logger::Trace("Selection Dragging");
        Logger::Trace("Begin: ", viewData.selectionBegin);
        Logger::Trace("End:   ", viewData.selectionEnd);
        auto rect = viewData.getSelectionRect();
        auto* DL = ImGui::GetWindowDrawList();
        DL->AddRectFilled(rect.Min, rect.Max,
                          ImGui::ColorConvertFloat4ToU32(SelectionRectColor));
        DL->AddRect(rect.Min, rect.Max,
                    ImGui::ColorConvertFloat4ToU32(WhiteOutlineColor));
    }
    ImGui::EndChild();
}

void NodeEditor::onInput(InputEvent& event) { impl->onInput(event); }

void Impl::onInput(InputEvent& event) {
    using enum InputEventMask;
    event.dispatch<MouseDragged>([&](MouseDragEvent event) {
        if (viewData.isMoveDragging) {
            viewData.position += event.offset;
        }
        if (viewData.isSelectionDragging) {
            viewData.selectionEnd = event.locationInWindow;
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
    impl->currentNodeOrder.push_back(p.get());
    impl->graph.add(std::move(p));
}

Graph const& NodeEditor::graph() const { return impl->graph; }

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

static float4 selectPinColor(Pin const& pin) {
    auto* input = dyncast<InputPin const*>(&pin);
    if (!input || input->origin()) {
        return PinColorConnected;
    }
    if (test(input->desc().flags & PinFlags::Optional)) {
        return PinColorOptional;
    }
    return PinColorMissing;
}

static void drawPin(Pin const& pin, float2 position) {
    auto* DL = ImGui::GetWindowDrawList();
    float radius = 6;
    DL->AddCircleFilled(position + PinSize / 2, radius,
                        ImGui::ColorConvertFloat4ToU32(selectPinColor(pin)));
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
    float2 basePos = (float2)viewData.clipRect.Min + viewData.position;
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
    drawPin(pin, position);
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
    float pinHeight =
        PinSize * std::max(node.inputs().size(), node.outputs().size());
    return vml::max(node.minSize(), float2(0, pinHeight)) + 2 * bodyPadding;
}

static void displayInputsOutputs(ViewData& viewData, Node& node) {
    float2 nodepos = (float2)ImGui::GetWindowPos() + NodeOuterPadding;
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

/// \Returns the area in which content can be drawn
static ImRect drawNodeBody(Node const& node, bool selected) {
    float2 pos = (float2)ImGui::GetWindowPos() + NodeOuterPadding;
    float rounding = 6;
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(pos, pos + node.size(),
                      ImGui::ColorConvertFloat4ToU32(node.desc().color),
                      rounding + 1);
    if (selected) {
        DL->AddRect(pos + 2, pos + node.size() - 2,
                    ImGui::ColorConvertFloat4ToU32(WhiteOutlineColor),
                    rounding - 1.5f, ImDrawFlags_None, 3);
    }
    DL->AddRect(pos + 1, pos + node.size() - 1,
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
    float contentPaddingLeft = node.inputs().empty() ? 0.0f : PinSize * 0.6f;
    float contentPaddingRight = node.outputs().empty() ? 0.0f : PinSize * 0.6f;
    return ImRect(pos + NodeBodyPadding +
                      float2(contentPaddingLeft,
                             labelSize.y + NodeBodyPadding.y),
                  pos + node.size() - NodeBodyPadding -
                      float2(contentPaddingRight, 0));
}

static bool displayNodeContent(ImRect contentRect, Node const& node) {
    if (!node.desc().content) {
        return false;
    }
    if (!beginInvisibleWindow("Content", contentRect.Min,
                              contentRect.GetSize()))
    {
        ImGui::EndChild();
        return false;
    }
    ImGui::SetNextItemAllowOverlap();
    bool bgPressed = ImGui::InvisibleButton("Content_View_Background_Button",
                                            ImGui::GetContentRegionAvail(),
                                            ImGuiButtonFlags_PressedOnClick);
    ImGui::SetCursorPos({});
    node.desc().content();
    ImGui::EndChild();
    return bgPressed;
}

static void displayNodeResizeGrip(ViewData& viewData, Node& node) {
    float2 areaSize = { 10, 10 };
    float2 pos = (float2)ImGui::GetWindowPos() + NodeOuterPadding +
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
        float2 minSize = computeMinNodeSize(node, NodeBodyPadding);
        viewData.resizeSize += ImGui::GetIO().MouseDelta;
        float2 newSize = vml::max(viewData.resizeSize, minSize);
        node.setSize(newSize);
    }
    if (hovered || dragging) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
    }
}

void Impl::displayNodeBody(Node& node) {
    float2 pos =
        (float2)ImGui::GetWindowPos() + viewData.position + node.position();
    if (!beginInvisibleWindow("Body", pos - NodeOuterPadding,
                              node.size() + 2 * NodeOuterPadding))
    {
        ImGui::EndChild();
        return;
    }
    bool isSelected = selection.contains(&node);
    auto* DL = ImGui::GetWindowDrawList();
    DL->PushClipRect(viewData.clipRect.Min, viewData.clipRect.Max);
    ImRect contentRect = drawNodeBody(node, isSelected);
    ImGuiID id = ImGui::GetID("Node_Body");
    ImRect bb(pos, pos + node.size());
    ImGui::ItemSize(node.size());
    ImGui::ItemAdd(bb, id);
    bool bgClicked = ImGui::IsItemClicked();
    displayInputsOutputs(viewData, node);
    bgClicked |= displayNodeContent(contentRect, node);
    displayNodeResizeGrip(viewData, node);
    if (bgClicked) {
        orderFront(&node);
        viewData.activeNode = &node;
    }
    /// Selected nodes change selection state when mouse is released
    if (isSelected && viewData.activeNode == &node &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
        !viewData.isDraggingNode && !viewData.handledSelectionSinceMousePresed)
    {
        selection.amend(&node);
    }
    /// Not selected nodes change selection state when mouse is pressed
    if (!isSelected && bgClicked) {
        /// We use this flag so we don't select and immediately unselect when
        /// the mouse is released
        viewData.handledSelectionSinceMousePresed = true;
        selection.amend(&node);
    }
    bool dragging = [&] {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            return false;
        }
        return viewData.activeNode == &node && !viewData.activeResize &&
               !viewData.activePin;
    }();
    if (dragging) {
        float2 delta = ImGui::GetIO().MouseDelta;
        if (vml::max_norm(delta) >= 1) {
            viewData.isDraggingNode = true;
        }
        for (auto* node: selection.nodes()) {
            node->setPosition(node->position() + delta);
        }
    }
    DL->PopClipRect();
    ImGui::EndChild();
}

void Impl::displayNode(Node& node) {
    node.setSize(
        vml::max(node.size(), computeMinNodeSize(node, NodeBodyPadding)));
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
    auto* window = ImGui::GetCurrentWindow();
    viewData.clipRect = window->ContentRegionRect;
    drawBackground(viewData.position);
    /// We make a copy here because we may reorder the nodes during traversal.
    /// This may seem wasteful but I'm sure it's fine
    for (auto* node: std::vector{ currentNodeOrder }) {
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
    ImGui::EndChild();
}

void Impl::orderFront(Node* node) {
    auto itr = ranges::find(currentNodeOrder, node);
    BL_ASSERT(itr != currentNodeOrder.end());
    currentNodeOrder.erase(itr);
    currentNodeOrder.push_back(node);
}

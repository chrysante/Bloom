#include "Poppy/Editor/Views/NodeEditor.h"

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/utility.hpp>

#include "Bloom/Core/Debug.h"
#include "Poppy/UI/Font.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

using NodeDesc = NodeEditor::NodeDesc;
using Node = NodeEditor::Node;
using Pin = NodeEditor::Pin;
using PinType = NodeEditor::PinType;
using Impl = NodeEditor::Impl;

namespace {

enum class Axis { Horizontal, Vertical };

struct PersistentViewData {
    vml::float2 position = { 0.0, 0.0 };
    float zoom = 1.0;
};

struct ViewData: PersistentViewData {
    bool isBGHovered = false;
    bool isDraggingNode = false;
    Node const* hoveredNode = nullptr;
    Node const* activeNode = nullptr;
    Node const* activeResize = nullptr;
    Pin const* activePin = nullptr;
};

struct Selection {
    std::vector<size_t> indices;
};

} // namespace

Node::Node(NodeDesc desc): _desc(std::move(desc)) {
    _inputs = _desc.inputs | transform([](PinDesc const& pin) {
        return std::make_unique<Pin>(pin, PinType::Input);
    }) | ranges::to<std::vector>;
    _outputs = _desc.outputs | transform([](PinDesc const& pin) {
        return std::make_unique<Pin>(pin, PinType::Output);
    }) | ranges::to<std::vector>;
}

struct NodeEditor::Impl {
    void display();

    void onInput(bloom::InputEvent& event);

    std::vector<Node*> currentNodeOrder;
    std::vector<std::unique_ptr<Node>> nodes;
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
    impl->nodes.push_back(std::move(p));
}

static float4 const WhiteOutlineColor = { 1, 1, 1, 0.25 };
static float4 const BlackOutlineColor = { 1, 1, 1, 0.25 };
static float2 const MinNodeSize = { 80, 40 };
static float2 const NodeBodyPadding = { 5, 5 };
static float const PinSize = 20;

static void drawPin(float2 position) {
    auto* DL = ImGui::GetWindowDrawList();
    float radius = 6;
    DL->AddCircleFilled(position + PinSize / 2, radius, IM_COL32(0, 0, 0, 255));
    DL->AddCircle(position + PinSize / 2, radius,
                  ImGui::ColorConvertFloat4ToU32(WhiteOutlineColor));
}

static void displayPinLabel(std::string_view label, PinType type,
                            float2 pinPosition) {
    auto* DL = ImGui::GetWindowDrawList();
    auto* textBegin = label.data();
    auto* textEnd = textBegin + label.size();
    float2 textSize = ImGui::CalcTextSize(textBegin, textEnd);
    float ypos = PinSize / 2 - textSize.y / 2;
    float2 textPos = type == PinType::Input ?
                         pinPosition + float2(-textSize.x, ypos) :
                         pinPosition + float2(PinSize, ypos);
    DL->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), textBegin, textEnd);
}

static void displayPin(ViewData& viewData, Pin const& pin, float2 position) {
    ImGui::PushID(&pin);
    ImRect bb(position, position + float2(PinSize));
    ImGui::ItemSize(float2(PinSize));
    auto ID = ImGui::GetID("Pin_Handle");
    ImGui::ItemAdd(bb, ID);
    bool hovered = ImGui::ItemHoverable(bb, ID, ImGuiItemFlags_AllowOverlap);
    if (ImGui::IsItemClicked()) {
        viewData.activePin = &pin;
    }
    auto* DL = ImGui::GetWindowDrawList();
    bool dragging = viewData.activePin == &pin &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragging) {
        float2 begin = bb.GetCenter();
        float2 end = ImGui::GetMousePos();
        float2 mid = (begin + end) / 2;
        float2 area = vml::abs(end - begin);
        float crtlPointFactor =
            2.0f * std::atan(area.y / 100) / vml::constants<float>::pi;
        float ctrlPointX = (pin.type() == PinType::Input ? -1.0f : 1.0f) *
                           crtlPointFactor * std::clamp(area.x, 100.f, 200.0f);
        float2 beginControlPoint = begin + float2(ctrlPointX, 0);
        float2 endControlPoint = end - float2(ctrlPointX, 0);
        DL->PathLineTo(begin);
        DL->PathBezierQuadraticCurveTo(beginControlPoint, mid, 0);
        DL->PathBezierQuadraticCurveTo(endControlPoint, end, 0);
        DL->PathStroke(IM_COL32(255, 0, 255, 255), ImDrawFlags_None, 2);
    }
    if (hovered && !dragging &&
        (!viewData.activePin || viewData.activePin->type() != pin.type()))
    {
        displayPinLabel(pin.name(), pin.type(), position);
    }
    drawPin(position);
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
    float2 pos = winpos + viewData.position + node.position();
    ImGui::PushID("Inputs");
    for (auto [pinIndex, pin]: node.inputs() | enumerate) {
        float2 position = pos + float2(PinSize * -0.4, NodeBodyPadding.y) +
                          float2(0, 20 * pinIndex);
        displayPin(viewData, *pin, position);
    }
    ImGui::PopID();
    ImGui::PushID("Outputs");
    for (auto [pinIndex, pin]: node.outputs() | enumerate) {
        float2 position =
            pos + float2(node.size().x + PinSize * -0.6, NodeBodyPadding.y) +
            float2(0, 20 * pinIndex);
        displayPin(viewData, *pin, position);
    }
    ImGui::PopID();
}

static void drawNodeBody(ViewData const& viewData, Node const& node) {
    float2 winpos = ImGui::GetWindowPos();
    float2 pos = winpos + viewData.position + node.position();
    float rounding = 5;
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(pos, pos + node.size(), IM_COL32(255, 127, 0, 255),
                      rounding + 1);
    DL->AddRect(pos + float2(1, 1), pos + node.size() - float2(2, 2),
                ImGui::ColorConvertFloat4ToU32(WhiteOutlineColor),
                rounding - 1);
    DL->AddRect(pos, pos + node.size(),
                ImGui::ColorConvertFloat4ToU32(BlackOutlineColor), rounding);
    auto* label = node.name().data();
    auto* labelEnd = node.name().data() + node.name().size();
    float2 labelSize =
        vml::min((float2)ImGui::CalcTextSize(label, labelEnd),
                 node.size() - float2(PinSize + 2 * NodeBodyPadding.x, 0));
    float2 labelPos = pos + float2((node.size().x - labelSize.x) / 2, rounding);
    auto labelCol =
        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text));
    auto* font =
        FontManager::get(FontDesc::UIDefault().setWeight(FontWeight::Bold));
    ImGui::PushClipRect(labelPos, labelPos + labelSize, true);
    DL->AddText(font, ImGui::GetFontSize(), labelPos, labelCol, label,
                labelEnd);
    ImGui::PopClipRect();
}

static void displayNodeBody(ViewData& viewData, Node& node) {
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
    drawNodeBody(viewData, node);
    displayInputsOutputs(viewData, node);
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

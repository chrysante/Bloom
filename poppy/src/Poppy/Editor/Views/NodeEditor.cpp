#include "Poppy/Editor/Views/NodeEditor.h"

#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/view.hpp>

#include "Bloom/Core/Debug.h"
#include "Poppy/UI/Font.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

enum class Axis { Horizontal, Vertical };

struct PersistentViewData {
    vml::float2 position = { 0.0, 0.0 };
    float zoom = 1.0;
};

struct ViewData: PersistentViewData {
    bool isBGHovered = false;
    bool isDraggingNode = false;
    size_t hoveredNode = ~size_t{};
    size_t activeNode = ~size_t{};
    size_t activeResize = ~size_t{};
};

struct Selection {
    std::vector<size_t> indices;
};

} // namespace

using Node = NodeEditor::Node;
using Impl = NodeEditor::Impl;

static constexpr size_t InvalidIndex = ~size_t{};

struct NodeEditor::Impl {
    void display();

    void onInput(bloom::InputEvent& event);

    std::vector<Node> nodes;
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

std::span<Node const> NodeEditor::nodes() const { return impl->nodes; }

void NodeEditor::addNode(Node node) { impl->nodes.push_back(std::move(node)); }

static void drawNodeBody(ViewData const& viewData, Node const& node) {
    float2 winpos = ImGui::GetWindowPos();
    float2 pos = winpos + viewData.position + node.position;
    float rounding = 5;
    float2 padding = { 5, 5 };
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(pos, pos + node.size, IM_COL32(255, 127, 0, 255),
                      rounding);
    float outlineAlpha = ImGui::GetStyleColorVec4(ImGuiCol_Border).x;
    auto outlineCol =
        ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, outlineAlpha));
    DL->AddRect(pos, pos + node.size, outlineCol, rounding);
    auto* label = node.name.data();
    auto* labelEnd = node.name.data() + node.name.size();
    float2 labelSize = vml::min((float2)ImGui::CalcTextSize(label, labelEnd),
                                node.size - float2(2 * padding.x, 0));
    float2 labelPos = pos + float2((node.size.x - labelSize.x) / 2, rounding);
    auto labelCol =
        ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text));
    auto* font =
        FontManager::get(FontDesc::UIDefault().setWeight(FontWeight::Bold));
    DL->AddText(font, ImGui::GetFontSize(), labelPos, labelCol, label,
                labelEnd);
}

static void displayNodeBody(ViewData& viewData, Node& node, size_t index) {
    float2 winpos = ImGui::GetWindowPos();
    float2 pos = winpos + viewData.position + node.position;
    ImGuiID id = ImGui::GetID("Node_Body");
    ImRect bb(pos, pos + node.size);
    ImGui::ItemSize(node.size);
    ImGui::ItemAdd(bb, id);
    bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
    if (hovered) {
        viewData.hoveredNode = index;
    }
    if (ImGui::IsItemClicked()) {
        viewData.activeNode = index;
    }
    bool dragging = [&] {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) return false;
        return viewData.activeNode == index &&
               viewData.activeResize == InvalidIndex;
    }();
    if (dragging) {
        node.position += ImGui::GetIO().MouseDelta;
    }
    drawNodeBody(viewData, node);
}

static float2 const MinNodeSize = { 80, 40 };

static void displayNodeResizeGrip(ViewData& viewData, Node& node,
                                  size_t index) {
    float2 areaSize = { 10, 10 };
    float2 winpos = ImGui::GetWindowPos();
    float2 pos =
        winpos + viewData.position + node.position + node.size - areaSize;
    ImGuiID id = ImGui::GetID("Node_Resize_Grip");
    ImRect bb(pos, pos + areaSize);
    ImGui::ItemSize(areaSize);
    ImGui::ItemAdd(bb, id);
    bool hovered = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_AllowOverlap);
    if (ImGui::IsItemClicked()) {
        viewData.activeResize = index;
    }
    bool dragging = viewData.activeResize == index &&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (dragging) {
        node.size = vml::max(node.size + (float2)ImGui::GetIO().MouseDelta,
                             MinNodeSize);
    }
    if (hovered || dragging) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
    }
}

static void displayNode(ViewData& viewData, Node& node, size_t index) {
    ImGui::PushID((int)index);
    displayNodeBody(viewData, node, index);
    displayNodeResizeGrip(viewData, node, index);
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
    viewData.hoveredNode = InvalidIndex;
    for (auto [index, node]: nodes | enumerate) {
        displayNode(viewData, node, index);
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        viewData.activeNode = InvalidIndex;
        viewData.activeResize = InvalidIndex;
    }
    ImGui::EndChild();
}

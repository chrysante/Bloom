#include "Poppy/Editor/Views/RenderGraphView.h"

#include <imgui.h>

using namespace poppy;
using namespace bloom;

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

void RenderGraphView::init() {
    // clang-format off
    nodeEditor.addNode({
        .name = "Some Input",
        .position = { 0, 0 },
        .minSize = { 80, 0 },
        .inputs = { { .name = "Optional Input" } },
        .outputs = { { .name = "Texture" } }
    });
    nodeEditor.addNode({
        .name = "Some Long Pass",
        .position = { 200, 400 },
        .minSize = { 150, 100 },
        .inputs = { { .name = "Depth Buffer" }, { .name = "Raw Image" } },
        .outputs = { { .name = "Result" } },
        .content = [] {
            if (ImGui::Button("Button")) {
                Logger::Info("Pressed the button");
            }
        },
        .color = { 0, 0.5, 1, 1 }
    });
    nodeEditor.addNode({
        .name = "Another Pass",
        .position = { 100, 300 },
        .minSize = { 100, 80 },
        .outputs = { { .name = "Result" } },
        .content = [] {
            if (ImGui::Button("Another Button")) {
                Logger::Info("Pressed the other button");
            }
        },
        .color = { 1, 0.5, 0, 1 }
    });
    // clang-format on
}

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

void RenderGraphView::frame() { nodeEditor.display(); }

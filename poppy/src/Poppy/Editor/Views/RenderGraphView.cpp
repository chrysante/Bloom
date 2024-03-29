#include "Poppy/Editor/Views/RenderGraphView.h"

#include <imgui.h>
#include <vml/vml.hpp>

using namespace poppy;
using namespace bloom;
using namespace vml;

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

void RenderGraphView::init() {
    using namespace nodeEditor;

    static constexpr float4 InputColor = { 0.4, 0.7, 0.2, 1.0 };
    static constexpr float4 PassColor = { 0.3, 0.0, 0.8, 1.0 };
    static constexpr float4 OutputColor = { 1.0, 0.0, 0.2, 1.0 };

    nodeEditor.setDelegate({ .backgroundPopupMenu = [] {
        if (ImGui::BeginMenu("Inputs")) {
            if (ImGui::MenuItem("Scene Geometry")) {
            }
            if (ImGui::MenuItem("Scene Lights")) {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Outputs")) {
            if (ImGui::MenuItem("Swapchain Present")) {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Passes")) {
            if (ImGui::MenuItem("Shadow Pass")) {
            }
            ImGui::EndMenu();
        }
    } });

    // clang-format off
    nodeEditor.addNode({
        .name = "Scene Geometry",
        .position = { 100, 100 },
        .minSize = { 80, 0 },
        .inputs = {},
        .outputs = { { .name = "Data" } },
        .color = InputColor
    });
    nodeEditor.addNode({
        .name = "Scene Lights",
        .position = { 100, 200 },
        .minSize = { 80, 0 },
        .inputs = {},
        .outputs = { { .name = "Data" } },
        .color = InputColor
    });
    nodeEditor.addNode({
        .name = "Shadow Pass",
        .position = { 200, 400 },
        .minSize = { 150, 100 },
        .inputs = {
            { .name = "Geometry" },
            { .name = "Lights" }
        },
        .outputs = { { .name = "Shadow Map" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Main Pass",
        .position = { 200, 600 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Geometry" }, { .name = "Lights" }, { .name = "Shadow Map" } },
        .outputs = { { .name = "Raw Color" }, { .name = "Depth Buffer" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Bloom Pass",
        .position = { 200, 700 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Raw Color" } },
        .outputs = { { .name = "Glow Color" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Post Process Pass",
        .position = { 200, 800 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Raw Color" }, { .name = "Depth" }, { .name = "Glow Color" } },
        .outputs = { { .name = "Final Color" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Swapchain Present",
        .position = { 200, 800 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Color" } },
        .color = OutputColor,
    });
    // clang-format on
}

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

void RenderGraphView::frame() { nodeEditor.display(); }

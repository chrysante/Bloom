#include <functional>
#include <string>
#include <vector>

#include <imgui.h>
#include <utl/hashtable.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/Editor/Views/NodeEditor.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml;

namespace {

struct RenderGraphView: View {
    RenderGraphView();

    void init() override;

    void onInput(bloom::InputEvent& event) override;

    void frame() override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    Toolbar makeToolbar();

    RenderGraph* renderGraph;
    NodeEditor nodeEditor;
    Toolbar toolbar;
};

} // namespace

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

namespace {

struct PassEntry {
    std::string name;
};

struct PassRegistry {

    std::vector<PassEntry> entries;
};

}; // namespace

static void createDefaultGraph(NodeEditor& nodeEditor) {
    using namespace nodeEditor;
    static constexpr float4 InputColor = { 0.4, 0.7, 0.2, 1.0 };
    static constexpr float4 PassColor = { 0.3, 0.0, 0.8, 1.0 };
    static constexpr float4 OutputColor = { 1.0, 0.0, 0.2, 1.0 };
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
        .inputs = { { .name = "Geometry" },
                    { .name = "Lights" },
                    { .name = "Shadow Map" } },
        .outputs = { { .name = "Raw Color" },
                     { .name = "Depth Buffer" } },
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
        .inputs = { { .name = "Raw Color" },
                    { .name = "Depth" },
                    { .name = "Glow Color" } },
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

RenderGraphView::RenderGraphView(): toolbar(makeToolbar()) {
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
}

void RenderGraphView::init() {
    if (nodeEditor.graph().empty()) {
        createDefaultGraph(nodeEditor);
    }
}

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

void RenderGraphView::frame() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6, 6 });
    toolbar.display();
    ImGui::PopStyleVar();
    nodeEditor.display();
}

Toolbar RenderGraphView::makeToolbar() {
    // clang-format off
    return {
        ToolbarIconButton("resize-small")
            .onClick([this] {
                dispatch(DispatchToken::NextFrame,  [] {
                    Logger::Warn("Unimplemented");
                });
            })
    }; // clang-format on
}

YAML::Node RenderGraphView::serialize() const {
    /// We use these functions to serialize our graph for now. This is not
    /// really correct, these functions should only serialize view related data,
    /// we will change this in the future
    return nodeEditor.graph().serialize();
}

void RenderGraphView::deserialize(YAML::Node node) {
    if (!node) {
        return;
    }
    NodeEditorGraph graph;
    graph.deserialize(node);
    nodeEditor.setGraph(std::move(graph));
}

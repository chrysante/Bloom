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
#include "Poppy/Editor/Views/RenderGraphNodeController.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

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

RenderGraphView::RenderGraphView(): toolbar(makeToolbar()) {
    nodeEditor.setDelegate({ .backgroundPopupMenu = [&](float2 position) {
        for (auto& entry: PassRegistry::get()) {
            if (ImGui::MenuItem(entry.desc.name.c_str())) {
                auto desc = entry.desc;
                desc.position = position;
                nodeEditor.addNode(desc);
            }
        }
    } });
}

void RenderGraphView::init() {}

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
        ToolbarIconButton("flash")
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

void RenderGraphView::deserialize(YAML::Node) {
#if 0
    if (!node) {
        return;
    }
    NodeEditorGraph graph;
    graph.deserialize(node);
    nodeEditor.setGraph(std::move(graph));
#endif
}

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
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/NodeEditor.h"
#include "Poppy/Views/RenderGraphNodeController.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

enum CompilationStatus { None, Success, Error };

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
    CompilationStatus compilationStatus;
};

} // namespace

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

RenderGraphView::RenderGraphView(): toolbar(makeToolbar()) {
    nodeEditor.setDelegate({ .backgroundPopupMenu = [&](float2 position) {
        for (auto& entry: PassRegistry::get()) {
            if (ImGui::MenuItem(entry.name.c_str())) {
                auto desc = entry.factory();
                desc.position = position;
                nodeEditor.addNode(std::move(desc));
            }
        }
    } });
}

void RenderGraphView::init() { setPadding({}); }

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

void RenderGraphView::frame() {
    toolbar.display();
    nodeEditor.display();
}

static void createRenderGraph(HardwareDevice& device,
                              NodeEditorGraph const& model) {
    auto& ctx = RenderGraphManager::context();
    ctx.resourcePool.clear();
    auto rg = constructRenderGraph(ctx, device, model);
    RenderGraphManager::set(std::move(rg));
}

Toolbar RenderGraphView::makeToolbar() {
    // clang-format off
    return {
        ToolbarIconButton("flash")
            .onClick([this] {
                dispatch(DispatchToken::NextFrame,  [&] {
                    createRenderGraph(editor().device(), nodeEditor.model());
                });
            })
            .tooltip("Compile")
    }; // clang-format on
}

YAML::Node RenderGraphView::serialize() const {
    /// We use these functions to serialize our graph for now. This is not
    /// really correct, these functions should only serialize view related data,
    /// we will change this in the future
    return nodeEditor.model().serialize();
}

void RenderGraphView::deserialize(YAML::Node root) {
    if (!root) {
        return;
    }
    NodeEditorGraph graph;
    graph.deserialize(root, RenderGraphNodeController::deserialize);
    nodeEditor.setGraph(std::move(graph));
}

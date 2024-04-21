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
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/NodeEditor.h"
#include "Poppy/Views/RenderGraphNodeController.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace vml::short_types;

namespace {

enum CompilationStatus { None, Success, Error };

struct RenderGraphEditorView: View {
    RenderGraphEditorView();

    void init() override;

    void onInput(bloom::InputEvent& event) override;

    void display() override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    void compile();
    void doCompile();

    void backgroundContextMenu(float2 position);

    RenderGraph* renderGraph;
    NodeEditor nodeEditor;
    CompilationStatus compilationStatus = CompilationStatus::None;
};

} // namespace

POPPY_REGISTER_VIEW(RenderGraphEditorView, "Render Graph Editor");

static char const* compStatusMessage(CompilationStatus status) {
    using enum CompilationStatus;
    switch (status) {
    case None:
        return "";
    case Success:
        return "Compile succeeded";
    case Error:
        return "Compile failed";
    }
}

RenderGraphEditorView::RenderGraphEditorView():
    nodeEditor(commandPaletteWrite()) {
    nodeEditor.setDelegate({ .backgroundPopupMenu = [this](float2 pos) {
        backgroundContextMenu(pos);
    } });
    auto& cmd = commandPaletteWrite();
    cmd.add("Compile", { Key::B, ModFlags::Control }, [this] { compile(); });
    // clang-format off
    setToolbar({
        ToolbarIconButton(commandPalette().find("Compile"), "play"),
        ToolbarSpacer(),
        ToolbarTextItem()
            .text([this] { return compStatusMessage(compilationStatus); })
            .font(FontDesc::UIDefault().setSize(FontSize{ 16 }))
    }); // clang-format on
}

void RenderGraphEditorView::init() { setPadding({ 0, 0 }); }

void RenderGraphEditorView::onInput(InputEvent& event) {
    nodeEditor.onInput(event);
}

void RenderGraphEditorView::display() { nodeEditor.display(); }

YAML::Node RenderGraphEditorView::serialize() const {
    /// We use these functions to serialize our graph for now. This is not
    /// really correct, these functions should only serialize view related data,
    /// we will change this in the future
    return nodeEditor.model().serialize();
}

void RenderGraphEditorView::deserialize(YAML::Node root) {
    if (!root) {
        return;
    }
    NodeEditorGraph graph;
    graph.deserialize(root, RenderGraphNodeController::deserialize);
    nodeEditor.setGraph(std::move(graph));
}

void RenderGraphEditorView::compile() {
    dispatch(DispatchToken::NextFrame, [this] { doCompile(); });
}

void RenderGraphEditorView::doCompile() {
    auto& ctx = RenderGraphManager::context();
    ctx.resourcePool.clear();
    auto rg = constructRenderGraph(ctx, editor().device(), nodeEditor.model());
    if (rg) {
        compilationStatus = CompilationStatus::Success;
    }
    else {
        compilationStatus = CompilationStatus::Error;
    }
    RenderGraphManager::set(std::move(rg));
}

void RenderGraphEditorView::backgroundContextMenu(float2 position) {
    for (auto& entry: PassRegistry::get()) {
        if (ImGui::MenuItem(entry.name.c_str())) {
            auto desc = entry.factory();
            desc.position = position;
            nodeEditor.addNode(std::move(desc));
        }
    }
}

#include "Poppy/Editor/Views/RenderGraphView.h"

#include <imgui.h>

using namespace poppy;
using namespace bloom;

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

void RenderGraphView::init() {
    nodeEditor.addNode(
        { .name = "Some Input", .position = { 0, 0 }, .size = { 80, 40 } });
    nodeEditor.addNode({ .name = "Some Long Pass",
                         .position = { 200, 400 },
                         .size = { 80, 40 } });
}

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

void RenderGraphView::frame() { nodeEditor.display(); }

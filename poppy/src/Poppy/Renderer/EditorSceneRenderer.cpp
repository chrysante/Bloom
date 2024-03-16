#include "Poppy/Renderer/EditorSceneRenderer.h"

#include "Bloom/Graphics/StaticMesh.h"
#include "Bloom/Scene/Components/MeshRenderer.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/SelectionContext.h"
#include "Poppy/Renderer/EditorRenderer.h"

using namespace bloom;
using namespace poppy;

void EditorSceneRenderer::submitExtra() {
    if (!mSelection) {
        return;
    }
    auto* const editorRenderer = dynamic_cast<EditorRenderer*>(&renderer());
    if (!editorRenderer) {
        return;
    }
    for (auto entity: mSelection->entities()) {
        if (!entity.has<MeshRendererComponent>() ||
            !entity.has<TransformMatrixComponent>())
        {
            continue;
        }
        auto& meshRenderer = entity.get<MeshRendererComponent>();
        if (!meshRenderer.mesh) {
            continue;
        }
        editorRenderer
            ->submitSelected(meshRenderer.mesh->getRenderer(),
                             entity.get<TransformMatrixComponent>().matrix);
    }
}

void EditorSceneRenderer::drawOverlays(SelectionContext const& selection,
                                       OverlayDrawDescription const& drawDesc,
                                       Framebuffer& framebuffer,
                                       EditorFramebuffer& editorFramebuffer,
                                       CommandQueue& commandQueue) {
    mSelection = &selection;
    mDrawDesc = drawDesc;
    if (selection.empty()) {
        mDrawDesc.drawSelection = false;
    }
    if (auto* editorRenderer = dynamic_cast<EditorRenderer*>(&renderer())) {
        editorRenderer->drawOverlays(framebuffer, editorFramebuffer,
                                     commandQueue, drawDesc);
    }
}

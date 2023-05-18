#include "Poppy/Renderer/EditorSceneRenderer.hpp"

#include "Bloom/Graphics/StaticMesh.hpp"
#include "Bloom/Scene/Components/MeshRenderer.hpp"
#include "Bloom/Scene/Components/Transform.hpp"
#include "Bloom/Scene/Scene.hpp"
#include "Poppy/Editor/SelectionContext.hpp"
#include "Poppy/Renderer/EditorRenderer.hpp"

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

void EditorSceneRenderer::drawWithOverlays(
    std::span<Scene const* const> scenes,
    SelectionContext const& selection,
    Camera const& camera,
    OverlayDrawDescription const& drawDesc,
    Framebuffer& framebuffer,
    EditorFramebuffer& editorFramebuffer,
    CommandQueue& commandQueue) {
    mSelection = &selection;
    mDrawDesc  = drawDesc;
    if (selection.empty()) {
        mDrawDesc.drawSelection = false;
    }
    SceneRenderer::draw(scenes, camera, framebuffer, commandQueue);
    if (auto* const editorRenderer = dynamic_cast<EditorRenderer*>(&renderer()))
    {
        editorRenderer->drawOverlays(framebuffer,
                                     editorFramebuffer,
                                     commandQueue,
                                     drawDesc);
    }
}

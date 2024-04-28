#define IMGUI_DEFINE_MATH_OPERATORS

#include <memory>

#include <imgui.h>
#include <imgui_internal.h>

#include "Bloom/Graphics/Renderer/ForwardRenderer.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Renderer/EditorRenderer.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/Viewport/Viewport.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

namespace {

class RendererDebuggerView: public View {
public:
    void init() override;
    void display() override;

private:
    bloom::ForwardRenderer* findRenderer() const;

private:
    bloom::ForwardRenderer* mRenderer = nullptr;

private:
    struct BloomVeilParams {
        int downsampleMipmapLevel = 0;
        int upsampleMipmapLevel = 0;
    } veil;
};

} // namespace

POPPY_REGISTER_VIEW(RendererDebuggerView, "Renderer Debugger");

void RendererDebuggerView::init() {}

void RendererDebuggerView::display() {
    if (!mRenderer && !(mRenderer = findRenderer())) {
        displayEmptyWithReason("No Renderer Found");
        return;
    }

    using namespace propertiesView;
    if (beginSection("Postprocessing")) {
        beginProperty("Tone Mapping");
        fullWidth();
        char const* const items[] = { "ACES", "Reinhard" };
        ImGui::Combo("##tone-mapping", (int*)&mRenderer->mToneMapping, items,
                     2);
        endSection();
    }

    if (beginSection("Bloom/Veil")) {
        /* Bloom Settings */ {
            auto params = mRenderer->bloomRenderer.getParameters();
            bool edit = false;

            beginProperty("Enabled");
            edit |= ImGui::Checkbox("##enabled", &params.enabled);
            if (params.enabled) {
                beginProperty("Physically Correct");
                edit |= ImGui::Checkbox("##physically-correct",
                                        &params.physicallyCorrect);
                if (!params.physicallyCorrect) {
                    beginProperty("Intensity");
                    fullWidth();
                    edit |= ImGui::SliderFloat("##intensity", &params.intensity,
                                               0, 10);
                    beginProperty("Threshold");
                    fullWidth();
                    edit |= ImGui::SliderFloat("##threshold", &params.threshold,
                                               0, 10);
                    beginProperty("Soft Knee");
                    fullWidth();
                    edit |= ImGui::SliderFloat("##knee", &params.knee, 0, 1);
                }

                beginProperty("Clamp");
                fullWidth();
                edit |=
                    ImGui::DragFloat("##clamp", &params.clamp, 0.1, 0, FLT_MAX);
                beginProperty("Diffusion");
                fullWidth();
                edit |=
                    ImGui::SliderFloat("##diffusion", &params.diffusion, 0, 10);

                if (params.physicallyCorrect) {
                    beginProperty("Contribution");
                    fullWidth();
                    edit |= ImGui::SliderFloat("##contribution",
                                               &params.contribution, 0, 1);
                }
            }

            if (edit) {
                mRenderer->bloomRenderer.setParameters(params);
            }
        }
        endSection();
    }
}

bloom::ForwardRenderer* RendererDebuggerView::findRenderer() const {
    if (auto* editorRenderer =
            dynamic_cast<EditorRenderer*>(&editor().coreSystems().renderer()))
    {
        return dynamic_cast<bloom::ForwardRenderer*>(
            &editorRenderer->wrappedRenderer());
    }
    return dynamic_cast<bloom::ForwardRenderer*>(
        &editor().coreSystems().renderer());
}

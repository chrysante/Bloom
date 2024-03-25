#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/Editor/Views/RendererDebuggerView.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Views/Viewport/Viewport.h"
#include "Poppy/Renderer/EditorRenderer.h"
#include "Poppy/UI/PropertiesView.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

POPPY_REGISTER_VIEW(RendererDebuggerView, "Renderer Debugger", {});

void RendererDebuggerView::init() {
    mRenderer = findRenderer();
    mFramebuffer = findFramebuffer();
}

void RendererDebuggerView::frame() {
    currentFramebuffer.reset();
    currentFramebuffer = mFramebuffer.lock();

    if (!mRenderer && !(mRenderer = findRenderer())) {
        displayEmptyWithReason("No Renderer found");
        return;
    }

    if (!currentFramebuffer) {
        mFramebuffer = findFramebuffer();
        currentFramebuffer = mFramebuffer.lock();
        if (!currentFramebuffer) {
            displayEmptyWithReason("No Compatible Framebuffer found");
            return;
        }
    }

    using namespace propertiesView;
    if (beginSection("Postprocessing")) {
        beginProperty("Tone Mapping");
        fullWidth();
        char const* const items[] = { "ACES", "reinhard" };
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
        header("Downsample Pass", FontDesc::UIDefault());
        beginSection();
        beginProperty("Mipmap Level");
        fullWidth();
        ImGui::SliderInt("##downsample-mipmap-level",
                         &veil.downsampleMipmapLevel, 0,
                         BloomFramebuffer::numDSMipLevels - 1, nullptr,
                         ImGuiSliderFlags_AlwaysClamp);

        endSection();

        auto displayImage = [](TextureView image, int2 size) {
            auto& style = ImGui::GetStyle();
            auto* const window = ImGui::GetCurrentWindow();
            float2 const cp = window->DC.CursorPos;
            float const width = ImGui::GetContentRegionAvail().x;
            float2 const imageSize =
                float2(width, image.size().y / float(image.size().x) * width);
            float2 const cursorEnd =
                (float2)ImGui::GetCursorPos() +
                float2(0, imageSize.y + style.ItemSpacing.y);
            window->DrawList->AddImageRounded(image.nativeHandle(), cp,
                                              cp + imageSize, { 0, 0 },
                                              { 1, 1 }, 0xFFffFFff,
                                              style.FrameRounding);
            window->DrawList->AddRect(cp, cp + imageSize,
                                      ImGui::GetColorU32(ImGuiCol_Border),
                                      style.FrameRounding);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + style.FramePadding);
            ImGui::Text("%d x %d", size.x, size.y);
            ImGui::SetCursorPos(cursorEnd);
        };

        displayImage(currentFramebuffer->bloom
                         .downsampleMips[veil.downsampleMipmapLevel],
                     currentFramebuffer->size /
                         (1 << (veil.downsampleMipmapLevel + 1)));

        header("Upsample Pass", FontDesc::UIDefault());
        beginSection();
        beginProperty("Mipmap Level");
        fullWidth();
        ImGui::SliderInt("##upsample-mipmap-level", &veil.upsampleMipmapLevel,
                         0, BloomFramebuffer::numUSMipLevels - 1, nullptr,
                         ImGuiSliderFlags_AlwaysClamp);
        endSection();

        displayImage(currentFramebuffer->bloom
                         .upsampleMips[veil.upsampleMipmapLevel],
                     currentFramebuffer->size /
                         (1 << (veil.upsampleMipmapLevel + 1)));
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

std::shared_ptr<bloom::ForwardRendererFramebuffer> RendererDebuggerView::
    findFramebuffer() const {
    auto views = editor().getAllViews();
    for (auto* v: views) {
        auto* viewport = dynamic_cast<Viewport*>(v);
        if (!viewport) {
            continue;
        }
        auto const fwFramebuffer =
            std::dynamic_pointer_cast<bloom::ForwardRendererFramebuffer>(
                viewport->framebuffer);
        if (fwFramebuffer) {
            return fwFramebuffer;
        }
    }
    return nullptr;
}

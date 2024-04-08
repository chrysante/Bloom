#if 0

#include <array>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Core/Serialize.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/Editor/Views/Viewport/Gizmo.h"
#include "Poppy/Editor/Views/Viewport/ViewportCameraController.h"
#include "Poppy/Editor/Views/Viewport/ViewportOverlays.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

namespace {

struct Parameters {
    enum class FramebufferElements {
        Depth,
        Raw,
        Postprocessed,
        LAST = Postprocessed
    };
    FramebufferElements framebufferSlot =
        FramebufferElements::Postprocessed;
};

struct DebugDrawOptions {
    enum class Mode { Lit = 0, Wireframe, LAST = Wireframe };
    Mode mode = Mode::Lit;

    bool visualizeShadowCascades = false;
    bloom::EntityHandle lightVizEntity;
};

static std::string toString(DebugDrawOptions::Mode mode) {
    return std::array{ "Lit", "Wireframe" }[(size_t)mode];
};

struct Viewport2: View, BasicSceneInspector {
    Viewport2();
    void init() override;
    void frame() override;
    void displayScenes();
    void displayToolbar();
    void onInput(bloom::InputEvent&) override;
    YAML::Node serialize() const override;
    void deserialize(YAML::Node) override;
    
    Toolbar makeToolbar();
    
    Toolbar toolbar;
    Gizmo gizmo;
    Parameters params;
    DebugDrawOptions drawOptions;
    ViewportCameraController cameraController;
};

} // namespace

POPPY_REGISTER_VIEW(Viewport2, "Viewport2", {});

Viewport2::Viewport2(): BasicSceneInspector(this), toolbar(makeToolbar()) {}

void Viewport2::init() {
    setPadding(0);
    toolbar.setStyle({
        .height = 25,
        .buttonAlpha = 1,
        .buttonAlphaHovered = 1,
        .buttonAlphaActive = 1,
    });
    gizmo.setInput(window().input());
}

static ViewportInput detectViewportInput(ImGuiButtonFlags buttonFlags) {
    using namespace ImGui;
    buttonFlags |= ImGuiButtonFlags_NoNavFocus;
    // Save to restore later
    ImVec2 cursorPos = GetCursorPos();
    ImGuiID hovered = ImGui::GetCurrentContext()->HoveredId;
    SetCursorPos({ 0, 0 });
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) return ViewportInput{};
    ImGuiID id = window->GetID("__Invisible_Button__");
    ImVec2 size = CalcItemSize(window->Size, 0.0f, 0.0f);
    ImRect bb(window->DC.CursorPos,
              vml::float2(window->DC.CursorPos) + vml::float2(size));
    ItemSize(size);
    if (!ItemAdd(bb, id)) return {};
    ViewportInput result{};
    result.pressed =
        ButtonBehavior(bb, id, &result.hovered, &result.held, buttonFlags);
    ImGui::SetHoveredID(hovered);
    SetCursorPos(cursorPos);
    return result;
}

void Viewport2::frame() {
    if (scenes().empty()) {
        displayEmptyWithReason("No Active Scene");
    }
    else {
        displayScenes();
    }
    displayToolbar();
    if (!scenes().empty()) {
        auto wantsInput =
            ::detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
        viewportHovered = wantsInput.hovered;
        if (wantsInput.held) {
            camera.update(Timestep{ 0, 0.166 }, window().input());
        }
    }
    if (!isSimulating() && GImGui->DragDropActive) {
        float spacing = 6.5;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::SetCursorPos({ spacing, spacing });
        ImGui::InvisibleButton("viewport-drag-drop-button",
                               ImGui::GetContentRegionAvail() -
                                   ImVec2(spacing, spacing));
        ImGui::PopItemFlag();
        receiveSceneDragDrop();
    }
}

void Viewport2::displayToolbar() {
    float2 padding = ImGui::GetStyle().WindowPadding;
    ImGui::SetCursorPos(padding);
    float4 color = float4(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)) * float4(1, 1, 1, 0.5);
    auto* DL = ImGui::GetWindowDrawList();
    float2 winpos = ImGui::GetWindowPos();
    float2 winsize = ImGui::GetWindowSize();
    DL->AddRectFilled(winpos,
                      winpos + float2(winsize.x, toolbar.getHeight() + 2 * padding.y),
                      ImGui::ColorConvertFloat4ToU32(color));
    toolbar.display(size().x - 2 * padding.x);
}

void Viewport2::onInput(bloom::InputEvent&) {
    
}

YAML::Node Viewport2::serialize() const {
    
}

void Viewport2::deserialize(YAML::Node) {
    
}

Toolbar Viewport2::makeToolbar() {
    // clang-format off
    return {
        ToolbarDropdownMenu()
            .content([this] {
                dropdownMenu();
            }).minWidth(400),

        ToolbarSpacer{},

        ToolbarDropdownMenu()
            .content([this] {
                enumCombo(drawOptions.mode);
            })
            .previewValue([this] {
                return toString(drawOptions.mode);
            }),
        ToolbarDropdownMenu()
            .content([this] {
                auto operation = gizmo.operation();
                if (enumCombo(operation)) {
                    gizmo.setOperation(operation);
                }
            })
            .previewValue([this] {
                return toString(gizmo.operation());
            }),

        ToolbarDropdownMenu()
            .content([this] {
                auto space = gizmo.space();
                if (enumCombo(space)) {
                    gizmo.setSpace(space);
                }
            }).previewValue([this] {
                return toString(gizmo.space());
            }),

        ToolbarDropdownMenu()
            .content([this] {
                enumCombo(camera.projection(),
                          [&](auto proj) { camera.setProjection(proj); });
            })
            .previewValue([this] {
                return toString(camera.projection());
            }),

        ToolbarSpacer{},

        ToolbarIconButton([this] {
            return maximized() ? "resize-small" : "resize-full";
        })
        .onClick([this] {
            dispatch(DispatchToken::NextFrame, [maximized = maximized(), this] {
                if (maximized) {
                    restore();
                    window().makeWindowed();
                }
                else {
                    maximize();
                    window().makeFullscreen();
                }
            });
        })
    }; // clang-format on
}

#endif

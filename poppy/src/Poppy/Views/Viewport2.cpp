#include <array>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/RenderGraph/SceneRenderer.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/Viewport/Gizmo.h"
#include "Poppy/Views/Viewport/ViewportCameraController.h"
#include "Poppy/Views/Viewport/ViewportOverlays.h"

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
    FramebufferElements framebufferSlot = FramebufferElements::Postprocessed;
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

struct MouseState {
    bool hovered, held, pressed;
};

struct Viewport2: View, BasicSceneInspector {
    Viewport2();
    void init() override;
    void display() override;
    void displayScenes();
    void displayToolbar() override {}
    void displayToolbarWithBackground();
    void readInput();
    void acceptDragDrop();
    void receiveSceneDragDrop();
    void onInput(bloom::InputEvent&) override;
    YAML::Node serialize() const override;
    void deserialize(YAML::Node) override;
    Toolbar makeToolbar();
    void dropdownMenu();

    Gizmo gizmo;
    Parameters params;
    DebugDrawOptions drawOptions;
    ViewportCameraController cameraController;
    MouseState mouseState;
};

} // namespace

POPPY_REGISTER_VIEW(Viewport2, "Viewport2");

Viewport2::Viewport2(): BasicSceneInspector(this) {
    cameraController.setFieldOfView(60);
}

void Viewport2::init() {
    setPadding({ 0, 0 });
    setToolbar(makeToolbar());

    gizmo.setInput(editor().input());
}

void Viewport2::display() {
    ImGui::BeginChild("Child");
    if (!scenes().empty()) {
        displayScenes();
    }
    else {
        displayEmptyWithReason("No Active Scene");
    }
    displayToolbarWithBackground();
    readInput();
    acceptDragDrop();
    ImGui::EndChild();
}

static rg::SceneRenderer& getSceneRenderer(rg::RenderGraph& renderGraph) {
    static rg::RenderGraph const* graph = nullptr;
    static std::unique_ptr<rg::SceneRenderer> instance;
    if (graph != &renderGraph) {
        graph = &renderGraph;
        instance = std::make_unique<rg::SceneRenderer>(&renderGraph);
    }
    return *instance;
}

void Viewport2::displayScenes() {
    auto windows = editor().getWindows();
    if (windows.empty()) {
        return;
    }
    float2 viewportSize = ImGui::GetContentRegionAvail();
    rg::FrameParameters frameParams = { .screenSize = viewportSize };
    auto* renderGraph = rg::RenderGraphManager::get();
    if (!renderGraph) {
        displayEmptyWithError("No Render Graph Found");
        return;
    }
    auto& renderer = getSceneRenderer(*renderGraph);
    cameraController.applyProjection(viewportSize);
    renderer.begin(frameParams, cameraController.camera());
    auto& sceneSystem = editor().coreSystems().sceneSystem();
    std::unique_lock lock = sceneSystem.lock();
    sceneSystem.applyTransformHierarchy();
    for (auto* scene: sceneSystem.scenes()) {
        renderer.submit(*scene);
    }
    lock.unlock();
    renderer.draw(windows.front()->commandQueue());
    if (renderGraph->sources().empty()) {
        displayEmptyWithError("Render Graph Has No Outputs");
        return;
    }
    auto* output =
        dyncast<rg::TextureResource*>(renderGraph->source(0)->resource());
    if (!output) {
        displayEmptyWithError("Render Graph Output Is Null");
        return;
    }
    ImGui::Image(output->handle().nativeHandle(),
                 ImGui::GetContentRegionAvail());
}

void Viewport2::displayToolbarWithBackground() {
    float4 color = float4(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)) *
                   float4(1, 1, 1, 0.5);
    auto* DL = ImGui::GetWindowDrawList();
    float2 winpos = ImGui::GetWindowPos();
    float2 winsize = ImGui::GetWindowSize();
    DL->AddRectFilled(winpos,
                      winpos + float2(winsize.x, toolbar()->getHeight()),
                      ImGui::ColorConvertFloat4ToU32(color));
    ImGui::SetCursorPos({});
    ImGui::SetCursorPos({});
    View::displayToolbar();
}

static MouseState detectViewportInput(ImGuiButtonFlags buttonFlags) {
    buttonFlags |= ImGuiButtonFlags_NoNavFocus;
    // Save to restore later
    ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGuiID hovered = ImGui::GetCurrentContext()->HoveredId;
    ImGui::SetCursorPos({ 0, 0 });
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return MouseState{};
    ImGuiID id = window->GetID("__Invisible_Button__");
    ImVec2 size = ImGui::CalcItemSize(window->Size, 0.0f, 0.0f);
    ImRect bb(window->DC.CursorPos,
              vml::float2(window->DC.CursorPos) + vml::float2(size));
    ImGui::ItemSize(size);
    if (!ImGui::ItemAdd(bb, id)) return {};
    MouseState result{};
    result.pressed = ImGui::ButtonBehavior(bb, id, &result.hovered,
                                           &result.held, buttonFlags);
    ImGui::SetHoveredID(hovered);
    ImGui::SetCursorPos(cursorPos);
    return result;
}

void Viewport2::readInput() {
    if (scenes().empty()) {
        return;
    }
    mouseState = ::detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
    if (mouseState.held) {
        cameraController.update(Timestep{ 0, 0.166 }, editor().input());
    }
}

void Viewport2::acceptDragDrop() {
    if (isSimulating() || !GImGui->DragDropActive) {
        return;
    }
    float spacing = 6.5;
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::SetCursorPos({ spacing, spacing });
    ImGui::InvisibleButton("viewport-drag-drop-button",
                           (float2)ImGui::GetContentRegionAvail() -
                               float2(spacing, spacing));
    ImGui::PopItemFlag();
    receiveSceneDragDrop();
}

void Viewport2::receiveSceneDragDrop() {
    auto payload = acceptAssetDragDrop({ AssetType::Scene });
    if (!payload) {
        return;
    }
    AssetHandle const handle = *payload;
    auto& AM = assetManager();
    auto asset = AM.get(handle);
    AM.makeAvailable(handle, AssetRepresentation::CPU);
    auto& sceneSystem = editor().coreSystems().sceneSystem();
    sceneSystem.unloadAll();
    sceneSystem.loadScene(as<Scene>(asset));
}

void Viewport2::onInput(bloom::InputEvent&) {}

YAML::Node Viewport2::serialize() const {
    YAML::Node root;
    root["CameraController"] = cameraController;
    return root;
}

void Viewport2::deserialize(YAML::Node node) {
    cameraController = node["CameraController"].as<ViewportCameraController>();
}

Toolbar Viewport2::makeToolbar() {
    // clang-format off
    Toolbar toolbar{
        ToolbarDropdownMenu()
            .content([this] { dropdownMenu(); })
            .minWidth(400),

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
                enumCombo(cameraController.projection(), [&](auto proj) {
                    cameraController.setProjection(proj);
                });
            })
            .previewValue([this] {
                return toString(cameraController.projection());
            }),

        ToolbarSpacer{},

        ToolbarIconButton([this] {
            return maximized() ? "resize-small" : "resize-full";
        })
            .onClick([this] {
                dispatch(DispatchToken::NextFrame, 
                         [maximized = maximized(), this] {
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
    toolbar.setStyle({
        .buttonAlpha = 1,
        .buttonAlphaHovered = 1,
        .buttonAlphaActive = 1,
    });
    return toolbar;
}

void Viewport2::dropdownMenu() {}

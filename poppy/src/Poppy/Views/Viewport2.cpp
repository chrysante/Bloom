#include <array>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/vector.hpp>
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
#include "Poppy/Renderer/EditorSceneRenderer2.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/Gizmo.h"
#include "Poppy/Views/LightOverlays.h"
#include "Poppy/Views/ViewportExt.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

static void displayRenderGraphDebugger(rg::RenderGraph const& rg, bool* open);

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
    void displayGizmo();

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

    float3 worldSpaceToViewSpace(float3 position);
    float3 worldSpaceToScreenSpace(float3 position);
    float2 worldSpaceDirToViewSpace(float3 direction);
    float2 worldSpaceDirToScreenSpace(float3 direction);

    Gizmo gizmo;
    Parameters params;
    DebugDrawOptions drawOptions;
    ViewportCameraController cameraController;
    MouseState mouseState;
    bool showRenderGraphDebugger = false;
};

} // namespace

POPPY_REGISTER_VIEW(Viewport2, "Viewport2");

Viewport2::Viewport2(): BasicSceneInspector(this) {
    cameraController.setFieldOfView(60);
}

void Viewport2::init() {
    setPadding({ 0, 0 });
    setToolbar(makeToolbar());
}

void Viewport2::display() {
    ImGui::BeginChild("Child");
    if (!scenes().empty()) {
        readInput();
        displayScenes();
        displayLightOverlays(scenes(), selection(), [&](float3 WS) {
            return worldSpaceToScreenSpace(WS);
        });
        displayGizmo();
    }
    else {
        displayEmptyWithReason("No Active Scene");
    }
    displayToolbarWithBackground();
    acceptDragDrop();
    ImGui::EndChild();
    if (showRenderGraphDebugger && rg::RenderGraphManager::get()) {
        displayRenderGraphDebugger(*rg::RenderGraphManager::get(),
                                   &showRenderGraphDebugger);
    }
}

static EditorSceneRenderer2& getSceneRenderer(rg::RenderGraph& renderGraph) {
    static rg::RenderGraph const* graph = nullptr;
    static std::unique_ptr<EditorSceneRenderer2> instance;
    if (graph != &renderGraph) {
        graph = &renderGraph;
        instance = std::make_unique<EditorSceneRenderer2>(&renderGraph);
    }
    return *instance;
}

void Viewport2::displayScenes() {
    float2 viewportSize =
        (float2)ImGui::GetContentRegionAvail() * window().contentScaleFactor();
    rg::FrameParameters frameParams = { .screenSize = viewportSize,
                                        .shadowMapSize = { 1024, 1024 } };
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
        renderer.submit(*scene, selection());
    }
    lock.unlock();
    renderer.draw(*window().commandQueue());
    if (renderGraph->sources().empty()) {
        displayEmptyWithError("Render Graph Has No Outputs");
        return;
    }
    auto finalColor = renderer.finalColor();
    if (!finalColor) {
        displayEmptyWithError("Render Graph Output Is Null");
        return;
    }
    ImGui::Image(finalColor.nativeHandle(), ImGui::GetContentRegionAvail());
}

void Viewport2::displayGizmo() {
    if (selection().empty()) {
        return;
    }
    EntityHandle entity = *selection().entities().begin();
    Transform transformComponent = entity.get<Transform>();
    EntityID parent = entity.getParent();
    auto& scene = entity.scene();
    auto localTransform = transformComponent.calculate();
    auto parentTransform = scene.calculateTransformRelativeToWorld(parent);
    auto entityWSTransform = parentTransform * localTransform;
    if (!gizmo.display(cameraController.camera(), entityWSTransform)) {
        return;
    }
    auto newLocalTransform = inverse(parentTransform) * entityWSTransform;
    entity.get<Transform>() = Transform::fromMatrix(newLocalTransform);
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
              float2(window->DC.CursorPos) + float2(size));
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

void Viewport2::dropdownMenu() {
    ImGui::Checkbox("Show Render Graph Debugger", &showRenderGraphDebugger);
}

static void displayRenderGraphDebugger(rg::RenderGraph const& rg, bool* open) {
    ImGui::Begin("Render Graph Debugger", open);
    if (ImGui::CollapsingHeader("Resource Pool",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& pool = rg.context().resourcePool;
        for (auto* resource: pool.resources()) {
            if (ImGui::TreeNodeEx((void*)resource,
                                  ImGuiTreeNodeFlags_DefaultOpen, "Resource"))
            {
                ImGui::TextEx(utl::strcat(resource->type()).c_str());
                if (ImGui::TreeNodeEx((void*)resource,
                                      ImGuiTreeNodeFlags_DefaultOpen, "Users"))
                {
                    for (auto* user: resource->users()) {
                        ImGui::TextEx(user->qualifiedName().c_str());
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}

float3 Viewport2::worldSpaceToViewSpace(float3 positionWS) {
    auto viewProj = cameraController.camera().viewProjection();
    auto tmp = viewProj * float4(positionWS, 1);
    auto ndc = tmp.xyz / tmp.w;
    auto posVS = ndc.xy;
    posVS = (posVS + 1) / 2;
    posVS.y = 1 - posVS.y;
    posVS *= this->size();
    return { posVS, ndc.z };
}

float3 Viewport2::worldSpaceToScreenSpace(float3 position) {
    auto vs = worldSpaceToViewSpace(position);
    return { viewSpaceToScreenSpace(vs.xy), vs.z };
}

float2 Viewport2::worldSpaceDirToViewSpace(float3 positionWS) {
    auto viewProj = cameraController.camera().viewProjection();
    auto tmp = viewProj * float4(positionWS, 0);
    auto ndc = tmp.xyz / tmp.w;
    auto dirVS = ndc.xy;
    dirVS.y = 1 - dirVS.y;
    dirVS *= this->size();
    return dirVS;
}

float2 Viewport2::worldSpaceDirToScreenSpace(float3 direction) {
    return worldSpaceDirToViewSpace(direction);
}

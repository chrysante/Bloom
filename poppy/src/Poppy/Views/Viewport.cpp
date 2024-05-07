#include <array>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/numeric.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Renderer/EditorSceneRenderer.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/Gizmo.h"
#include "Poppy/Views/ViewportExt.h"

using namespace poppy;
using namespace bloom;
using namespace rg;
using namespace ranges::views;
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

struct ActiveViewData {
    void reset() { *this = {}; }

    float4x4 gizmoWSTransform = 0;
};

struct ViewData: ActiveViewData {};

struct Viewport: View, BasicSceneInspector {
    Viewport();
    void init() override;

    void display() override;
    void drawScenes();
    void copyEntities(std::span<EntityHandle const> entities);
    void displayGizmo();
    void handleEntitySelection();

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
    EditorSceneRenderer* renderer = nullptr;
    ViewData viewData;
    bool showRenderGraphDebugger = false;
    bool gameView = false;
};

} // namespace

POPPY_REGISTER_VIEW(Viewport, "Viewport");

Viewport::Viewport(): BasicSceneInspector(this) {
    cameraController.setFieldOfView(60);
    auto& cmd = commandPaletteWrite();
    cmd.add("Game View", { Key::G }, [this] { gameView ^= true; });
    cmd.add("Cycle Gizmo", { Key::Tab }, [this] { gizmo.cycleSpace(); });
    cmd.add("Translate", { Key::Num1 },
            [this] { gizmo.setOperation(GizmoOperation::Translate); });
    cmd.add("Rotate", { Key::Num2 },
            [this] { gizmo.setOperation(GizmoOperation::Rotate); });
    cmd.add("Scale", { Key::Num3 },
            [this] { gizmo.setOperation(GizmoOperation::Scale); });
}

void Viewport::init() {
    setPadding({ 0, 0 });
    setToolbar(makeToolbar());
}

static EditorSceneRenderer& getSceneRenderer(rg::RenderGraph* renderGraph) {
    static rg::RenderGraph const* current = nullptr;
    static std::unique_ptr<EditorSceneRenderer> instance;
    static RenderGraphContext context;
    if (current != renderGraph) {
        current = renderGraph;
        context.resourcePool.clear();
        instance =
            std::make_unique<EditorSceneRenderer>(renderGraph->clone(context));
    }
    return *instance;
}

void Viewport::display() {
    ImGui::BeginChild("Child");
    if (!scenes().empty()) {
        readInput();
        drawScenes();
        if (!gameView) {
            auto WStoSS = [&](float3 WS) {
                return worldSpaceToScreenSpace(WS);
            };
            auto clickedEntity =
                displayLightOverlays(scenes(), selection(), WStoSS);
            displayGizmo();
            if (!gizmo.isUsing() && !gizmo.isHovered()) {
                if (clickedEntity) {
                    selection().amend(clickedEntity, getSelectionMode());
                }
                else {
                    handleEntitySelection();
                }
            }
        }
    }
    else {
        displayEmptyWithReason("No Active Scene");
    }
    displayToolbarWithBackground();
    acceptDragDrop();
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
    {
        viewData.reset();
    }
    ImGui::EndChild();
    if (showRenderGraphDebugger && renderer && renderer->renderGraph()) {
        displayRenderGraphDebugger(*renderer->renderGraph(),
                                   &showRenderGraphDebugger);
    }
}

void Viewport::drawScenes() {
    float2 viewportSize =
        (float2)ImGui::GetContentRegionAvail() * window().contentScaleFactor();
    rg::FrameParameters frameParams = {
        .screenSize = viewportSize,
        .shadowMapSize = { 1024, 1024 },
        .drawWireframe = drawOptions.mode == DebugDrawOptions::Mode::Wireframe
    };
    auto* renderGraph = rg::RenderGraphManager::get();
    if (!renderGraph) {
        displayEmptyWithError("No Render Graph Found");
        return;
    }
    renderer = &getSceneRenderer(renderGraph);
    cameraController.applyProjection(viewportSize);
    renderer->begin(frameParams, cameraController.camera());
    auto& sceneSystem = editor().coreSystems().sceneSystem();
    std::unique_lock lock = sceneSystem.lock();
    sceneSystem.applyTransformHierarchy();
    for (auto* scene: sceneSystem.scenes()) {
        renderer->submit(*scene, selection());
    }
    lock.unlock();
    renderer->draw(*window().commandQueue());
    if (renderGraph->sources().empty()) {
        displayEmptyWithError("Render Graph Has No Outputs");
        return;
    }
    auto finalColor = gameView ? renderer->sceneColor() :
                                 renderer->sceneColorWithOverlays();
    if (!finalColor) {
        displayEmptyWithError("Render Graph Output Is Null");
        return;
    }
    ImGui::Image(finalColor.nativeHandle(), ImGui::GetContentRegionAvail());
}

void Viewport::copyEntities(std::span<EntityHandle const> entities) {
    utl::hashmap<EntityHandle, EntityHandle> cloneMap;
    for (EntityHandle entity: entities) {
        cloneMap.insert({ entity, entity.clone() });
    }
    selection().transform([&](EntityHandle entity) {
        auto itr = cloneMap.find(entity);
        return itr != cloneMap.end() ? itr->second : entity;
    });
}

static float4x4 getDelta(float4x4 const& oldTransform,
                         float4x4 const& newTransform) {
    return newTransform * vml::inverse(oldTransform);
}

static float4x4 makeAverage(auto&& transforms) {
    struct SplitTransform {
        float3 position = { 0, 0, 0 };
        quaternion_float orientation =
            vml::make_rotation(0.0f, float3{ 0, 0, 1 });
        float3 scale = { 1, 1, 1 };
    };
    auto t = ranges::accumulate(transforms, SplitTransform{},
                                [&, i = 0](SplitTransform const& acc,
                                           float4x4 const& matrix) mutable {
        auto [position, orientation, scale] = vml::decompose_transform(matrix);
        float t = 1.0f - float(i++) / ranges::size(transforms);
        return SplitTransform{ vml::lerp(acc.position, position, t),
                               vml::slerp(acc.orientation, orientation, t),
                               vml::lerp(acc.scale, scale, t) };
    });
    return vml::make_transform(t.position, t.orientation, t.scale);
}

void Viewport::displayGizmo() {
    auto entities = selection().entities() | filter([](EntityHandle entity) {
        return entity.has<Transform>();
    }) | ToSmallVector<>;
    if (entities.empty()) {
        gizmo.reset();
        return;
    }
    auto localTransforms = entities | transform([](EntityHandle entity) {
        return entity.get<Transform>().calculate();
    }) | ToSmallVector<>;
    auto parentTransforms = entities | transform([](EntityHandle entity) {
        return entity.scene().calculateTransformRelativeToWorld(
            entity.getParent());
    }) | ToSmallVector<>;
    float4x4 avgWSTransform = [&] {
        if (!gizmo.isUsing()) {
            auto transforms = zip(localTransforms, parentTransforms) |
                              transform([](auto const& p) {
                auto& [local, parent] = p;
                return parent * local;
            });
            viewData.gizmoWSTransform = makeAverage(transforms);
        }
        return viewData.gizmoWSTransform;
    }();
    float4x4 oldTransform = avgWSTransform;
    bool isUsing = gizmo.isUsing();
    if (!gizmo.display(cameraController.camera(), avgWSTransform)) {
        return;
    }
    if (ImGui::IsKeyDown(ImGuiKey_ModShift) && !isUsing) {
        copyEntities(entities);
    }
    float4x4 transformDelta = getDelta(oldTransform, avgWSTransform);
    for (auto [entity, local, parent]:
         zip(entities, localTransforms, parentTransforms))
    {
        float4x4 newWSTransform = transformDelta * parent * local;
        auto newLocalTransform = vml::inverse(parent) * newWSTransform;
        entity.get<Transform>() = Transform::fromMatrix(newLocalTransform);
    }
    viewData.gizmoWSTransform = avgWSTransform;
}

void Viewport::handleEntitySelection() {
    if (!renderer || !ImGui::IsWindowHovered() ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        return;
    }
    float2 mousePos = window().contentScaleFactor() *
                      screenSpaceToViewSpace(ImGui::GetMousePos());
    EntityHandle entity =
        renderer->readEntityIDMap(*window().commandQueue(), scenes(), mousePos);
    auto mode = getSelectionMode();
    if (entity) {
        selection().amend(entity, mode);
    }
    else if (mode == SelectionMode::Replace) {
        selection().clear();
    }
}

void Viewport::displayToolbarWithBackground() {
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

void Viewport::readInput() {
    if (scenes().empty()) {
        return;
    }
    mouseState = ::detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
    if (mouseState.held) {
        cameraController.update(Timestep{ 0, 0.166 }, editor().input());
    }
}

void Viewport::acceptDragDrop() {
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

void Viewport::receiveSceneDragDrop() {
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

void Viewport::onInput(bloom::InputEvent&) {}

YAML::Node Viewport::serialize() const {
    YAML::Node root;
    root["CameraController"] = cameraController;
    return root;
}

void Viewport::deserialize(YAML::Node node) {
    cameraController = node["CameraController"].as<ViewportCameraController>();
}

Toolbar Viewport::makeToolbar() {
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

void Viewport::dropdownMenu() {
    using namespace propertiesView;
    ImGui::SetWindowSize({ ImGui::GetWindowSize().x,
                           std::min(ImGui::GetWindowSize().y, 250.0f) });
    header("Viewport Settings");
    if (beginSection()) {
        beginProperty("Game View");
        ImGui::Checkbox("###game-view", &gameView);
        beginProperty("Field Of View");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (float fov = cameraController.fieldOfView();
            ImGui::SliderFloat("###field-of-view", &fov, 30, 180))
        {
            cameraController.setFieldOfView(fov);
        }
        beginProperty("Near Clip Plane");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (float clip = cameraController.nearClipPlane();
            ImGui::SliderFloat("###near-clip-plane", &clip, 0, 1))
        {
            cameraController.setNearClipPlane(clip);
        }
        endSection();
    }
    ImGui::Checkbox("Show Render Graph Debugger", &showRenderGraphDebugger);
}

namespace bloom {

static std::ostream& operator<<(std::ostream& str, StorageMode mode) {
    switch (mode) {
    case StorageMode::Shared:
        return str << "Shared";
    case StorageMode::Managed:
        return str << "Managed";
    case StorageMode::GPUOnly:
        return str << "GPUOnly";
    default:
        return str << "<invalid>";
    }
}

} // namespace bloom

static void labeledText(std::string_view label, auto const&... args) {
    ImGui::TextEx(utl::strcat(label, ": ", args...).c_str());
}

static void displayResource(Resource const& resource) {
    if (!ImGui::TreeNodeEx((void*)&resource, ImGuiTreeNodeFlags_DefaultOpen,
                           "%s", utl::strcat(resource.type()).c_str()))
    {
        return;
    }
    // clang-format off
    visit(resource, utl::overload{
        [&](BufferResource const& resource) {
            if (!resource.handle()) {
                ImGui::Text("Not allocated");
                return;
            }
            labeledText("Size", resource.handle().size());
            labeledText("Storage Mode", resource.handle().storageMode());
        },
        [&](TextureResource const& resource) {
            if (!resource.handle()) {
                ImGui::Text("Not allocated");
                return;
            }
            labeledText("Size", resource.handle().size());
            labeledText("Storage Mode", 
                        resource.handle().description().storageMode);
        },
        [&](GeometryResource const&) {},
        [&](LightsResource const&) {}
    }); // clang-format on
    if (ImGui::TreeNodeEx((void*)&resource, ImGuiTreeNodeFlags_DefaultOpen,
                          "Users"))
    {
        for (auto* user: resource.users()) {
            withFont(FontDesc::MonoDefault(),
                     [&] { ImGui::TextEx(user->qualifiedName().c_str()); });
        }
        ImGui::TreePop();
    }
    ImGui::TreePop();
}

static void displayExecutionOrder(rg::RenderGraph const& rg) {
    for (auto* pass: rg.getExecutionOrder()) {
        auto* passRG = dynamic_cast<RenderGraph const*>(pass);
        bool open = withFont(FontDesc::MonoDefault(), [&] {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
            if (!passRG) {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            return ImGui::TreeNodeEx(pass, flags, "%s", pass->name().c_str());
        });
        if (!open) {
            continue;
        }
        if (passRG) {
            displayExecutionOrder(*passRG);
        }
        ImGui::TreePop();
    }
}

static void displayRenderGraphDebugger(rg::RenderGraph const& rg, bool* open) {
    ImGui::Begin("Render Graph Debugger", open);
    if (ImGui::CollapsingHeader("Execution Order",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        displayExecutionOrder(rg);
    }
    if (ImGui::CollapsingHeader("Resource Pool",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto& pool = rg.context().resourcePool;
        for (auto* resource: pool.resources()) {
            displayResource(*resource);
        }
    }
    ImGui::End();
}

float3 Viewport::worldSpaceToViewSpace(float3 positionWS) {
    auto viewProj = cameraController.camera().viewProjection();
    auto tmp = viewProj * float4(positionWS, 1);
    auto ndc = tmp.xyz / tmp.w;
    auto posVS = ndc.xy;
    posVS = (posVS + 1) / 2;
    posVS.y = 1 - posVS.y;
    posVS *= this->size();
    return { posVS, ndc.z };
}

float3 Viewport::worldSpaceToScreenSpace(float3 position) {
    auto vs = worldSpaceToViewSpace(position);
    return { viewSpaceToScreenSpace(vs.xy), vs.z };
}

float2 Viewport::worldSpaceDirToViewSpace(float3 positionWS) {
    auto viewProj = cameraController.camera().viewProjection();
    auto tmp = viewProj * float4(positionWS, 0);
    auto ndc = tmp.xyz / tmp.w;
    auto dirVS = ndc.xy;
    dirVS.y = 1 - dirVS.y;
    dirVS *= this->size();
    return dirVS;
}

float2 Viewport::worldSpaceDirToScreenSpace(float3 direction) {
    return worldSpaceDirToViewSpace(direction);
}

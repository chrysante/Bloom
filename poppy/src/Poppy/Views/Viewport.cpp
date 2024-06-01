#include <array>
#include <cstdlib>
#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/numeric.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Runtime/PhysicsSystem.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Shader/SharedTypes.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/Messages.h"
#include "Poppy/Editor/SceneTransactions.h"
#include "Poppy/Editor/TransactionManager.h"
#include "Poppy/Renderer/EditorSceneRenderer.h"
#include "Poppy/UI/PropertiesView.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/AssetDragDrop.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/Gizmo.h"
#include "Poppy/Views/TextureViewer.h"
#include "Poppy/Views/ViewportExt.h"

using namespace poppy;
using namespace bloom;
using namespace ranges::views;
using namespace vml::short_types;

static void displayRenderGraphDebugger(RenderGraph const& rg, bool* open);
static void displaySceneDebugger(Scene const& scene, bool* open);

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

struct RendererSettings {
    uint2 shadowMapResolution = { 512, 512 };
    BloomSettings bloomSettings;
    PostProcessSettings postProcessSettings;
};

static std::string toString(DebugDrawOptions::Mode mode) {
    return std::array{ "Lit", "Wireframe" }[(size_t)mode];
};

struct MouseState {
    bool hovered, held, pressed;
};

struct ActiveViewData {
    void reset() { *this = {}; }

    utl::small_vector<EditEntityComponent::WeakVariant> transformBackup;
    utl::small_vector<EntityHandle> originalEntitiesBackup;
};

struct ViewData: ActiveViewData {
    float4x4 gizmoWSTransform = 0;
};

struct Viewport: View, BasicSceneInspector {
    Viewport();
    void init() override;

    void display() override;

    void drawScene(Scene& scene, vml::rectangle<float> sceneRect);
    void displayScene();
    TextureHandle getSceneTexture();
    void displayOverlays(Scene& scene, vml::rectangle<float> sceneRect);
    void displayColorPicker(vml::rectangle<float> sceneRect);
    utl::small_vector<EntityHandle> copyEntities(
        std::span<EntityHandle const> entities);
    void displayGizmo(vml::rectangle<float> sceneRect);
    EntityID readEntityIDMap();
    void handleEntitySelection(EntityHandle clickedEntity);

    void displayToolbar() override {}
    void displayToolbarWithBackground();
    void readInput();
    void acceptDragDrop();
    void receiveSceneDragDrop();
    void openScene(AssetHandle handle);
    void openScene(Reference<Scene> scene);
    void onInput(bloom::InputEvent&) override;
    YAML::Node serialize() const override;
    void deserialize(YAML::Node) override;
    Toolbar makeToolbar();
    void displaySceneSettings(Scene& scene);
    void displayViewportSettings();
    void displayRendererSettings();
    void displayPhysicsSettings();

    void dropdownMenu();
    void debuggerMenu();

    void receiveData(std::any data) override;

    float3 worldSpaceToViewSpace(float3 position);
    float3 worldSpaceToScreenSpace(float3 position);
    float2 worldSpaceDirToViewSpace(float3 direction);
    float2 worldSpaceDirToScreenSpace(float3 direction);

    Gizmo gizmo;
    Parameters params;
    DebugDrawOptions drawOptions;
    ViewportCameraController cameraController;
    MouseState mouseState;
    std::unique_ptr<EditorSceneRenderer> renderer;
    ViewData viewData;
    TextureHandle colorPickerTmpTex;
    RendererSettings rendererSettings;
    bool showRenderGraphDebugger = false;
    bool showSceneDebugger = false;
    bool gameView = false;
    bool renderGraphDirty = true;
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

static AssetHandle getHandle(RenderGraph const* rg) {
    return rg ? rg->handle() : AssetHandle();
}

void Viewport::init() {
    setPadding({ 0, 0 });
    setToolbar(makeToolbar());
    editor().reactToSceneCommands(commandPaletteWrite());
    listen([this](DidRecompileRenderGraph msg) {
        auto* existing = renderer ? renderer->sceneRenderGraph() : nullptr;
        if (getHandle(existing) == getHandle(msg.renderGraph)) {
            renderGraphDirty = true;
        }
    });
    listen([this](UnloadSceneEvent) { renderGraphDirty = true; });
}

void Viewport::displayOverlays(Scene& scene, vml::rectangle<float> sceneRect) {
    auto WStoSS = [&](float3 WS) { return worldSpaceToScreenSpace(WS); };
    EntityHandle clickedEntity =
        displayLightOverlays(scene, selection(), WStoSS);
    displayGizmo(sceneRect);
    if (gizmo.isUsing() || gizmo.isHovered()) {
        return;
    }
    /// We don't want to select entities during simulation because the handles
    /// get invalidated when the simulation ends and we crash
    if (editor().isSimulating()) {
        return;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        ImGui::IsWindowHovered())
    {
        if (!clickedEntity) {
            auto ID = readEntityIDMap();
            if (scene.isValid(ID)) {
                clickedEntity = scene.getHandle(ID);
            }
        }
        handleEntitySelection(clickedEntity);
    }
}

static Scene* selectDisplayScene(SceneSystem const& sceneSystem) {
    if (sceneSystem.playScene()) {
        return sceneSystem.playScene();
    }
    if (sceneSystem.scene()) {
        return sceneSystem.scene();
    }
    return nullptr;
}

void Viewport::display() {
    vml::rectangle<float> sceneRect((float2)ImGui::GetWindowPos(),
                                    (float2)ImGui::GetWindowPos() +
                                        (float2)ImGui::GetWindowSize());
    auto* scene = selectDisplayScene(editor().sceneSystem());
    if (!scene) {
        if (renderGraphDirty) {
            renderer = nullptr;
            renderGraphDirty = false;
        }
        displayEmptyWithReason("No Active Scene");
        displayToolbarWithBackground();
        return;
    }
    displayScene();
    displayToolbarWithBackground();
    if (scene) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0);
        ImGui::BeginChild("Overlays");
        ImGui::PopStyleColor();
        readInput();
        if (!gameView && !editor().isSimulating()) {
            displayOverlays(*scene, sceneRect);
        }
        if (ImGui::IsWindowHovered() && ImGui::IsKeyDown(ImGuiMod_Alt)) {
            displayColorPicker(sceneRect);
        }
        ImGui::EndChild();
    }
    ///
    drawScene(*scene, sceneRect);
    acceptDragDrop();
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
    {
        viewData.reset();
    }
    if (showRenderGraphDebugger && renderer && renderer->renderGraph()) {
        displayRenderGraphDebugger(*renderer->renderGraph(),
                                   &showRenderGraphDebugger);
    }
    if (showSceneDebugger && scene) {
        displaySceneDebugger(*scene, &showSceneDebugger);
    }
}

void Viewport::displayColorPicker(vml::rectangle<float> sceneRect) {
    auto color = getSceneTexture();
    if (!color) {
        return;
    }
    float2 localMousePos =
        (float2)ImGui::GetMousePos() - sceneRect.lower_bound();
    float2 mouseCoord = localMousePos * color.size().xy / sceneRect.size();
    mouseCoord.x = std::clamp(mouseCoord.x, 0.0f, (float)color.size().x - 1);
    mouseCoord.y = std::clamp(mouseCoord.y, 0.0f, (float)color.size().y - 1);
    auto buffer = editor().commandQueue().createCommandBuffer();
    auto encoder = buffer->makeBlitEncoder();
    if (!colorPickerTmpTex || colorPickerTmpTex.description().pixelFormat !=
                                  color.description().pixelFormat)
    {
        auto desc = color.description();
        desc.size = { 1, 1, 1 };
        desc.storageMode = StorageMode::Managed;
        colorPickerTmpTex = editor().device().createTexture(desc);
    }
    encoder->copyTextureRegion(color, 0, 0, { mouseCoord, 0 }, { 1, 1, 1 },
                               colorPickerTmpTex, 0, 0, { 0, 0, 0 });
    encoder->synchronizeTexture(colorPickerTmpTex, 0, 0);
    encoder->end();
    displayTextureColorTooltip(colorPickerTmpTex, { 0, 0 }, mouseCoord);
    buffer->commit();
}

TextureHandle Viewport::getSceneTexture() {
    if (!renderer) {
        return {};
    }
    return gameView ? renderer->sceneColor() :
                      renderer->sceneColorWithOverlays();
}

void Viewport::displayScene() {
    if (!renderer || !renderer->renderGraph()) {
        displayEmptyWithError("No renderer");
        return;
    }
    auto* renderGraph = renderer->renderGraph();
    if (renderGraph->sources().empty()) {
        displayEmptyWithError("Render graph has no outputs");
        return;
    }
    auto sceneColor = getSceneTexture();
    if (!sceneColor) {
        displayEmptyWithError("Renderer output is null");
        return;
    }
    float2 size = ImGui::GetWindowSize();
    markObjectRenderHazard(sceneColor);
    auto* DL = ImGui::GetWindowDrawList();
    float2 pos = ImGui::GetWindowPos();
    DL->AddImage(sceneColor.nativeHandle(), pos, pos + size);
}

void Viewport::drawScene(Scene& scene, vml::rectangle<float> sceneRect) {
    FrameParameters frameParams = {
        .screenSize = sceneRect.size() * window().contentScaleFactor(),
        .shadowMapSize = rendererSettings.shadowMapResolution,
        .drawWireframe = drawOptions.mode == DebugDrawOptions::Mode::Wireframe
    };
    auto& sceneSystem = editor().sceneSystem();
    std::unique_lock sceneLock = sceneSystem.lock();
    if (renderGraphDirty || !renderer) {
        auto* renderGraph = scene.renderGraph();
        if (!renderGraph) {
            return;
        }
        renderer =
            std::make_unique<EditorSceneRenderer>(renderGraph->clone(),
                                                  editor().commandQueue());
        renderGraphDirty = false;
    }
    cameraController.applyProjection(sceneRect.size());
    renderer->begin(frameParams, cameraController.camera(),
                    rendererSettings.bloomSettings,
                    rendererSettings.postProcessSettings);
    scene.applyTransformHierarchy();
    renderer->submit(scene);
    if (!editor().isSimulating()) {
        renderer->submitSelection(selection().entities());
    }
    sceneLock.unlock();
    auto commandBuffer = editor().commandQueue().createCommandBuffer();
    renderer->draw(*commandBuffer);
    commandBuffer->commit();
}

utl::small_vector<EntityHandle> Viewport::copyEntities(
    std::span<EntityHandle const> entities) {
    utl::hashmap<EntityHandle, EntityHandle> cloneMap;
    for (EntityHandle entity: entities) {
        cloneMap.insert({ entity, entity.clone() });
    }
    selection().transform([&](EntityHandle entity) {
        auto itr = cloneMap.find(entity);
        return itr != cloneMap.end() ? itr->second : entity;
    });
    return selection().elements() | ToSmallVector<>;
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

static auto getWeakTransforms(std::span<EntityHandle const> entities) {
    return entities | transform([](EntityHandle e) {
        return EditEntityComponent::WeakVariant(e.get<Transform>());
    }) | ToSmallVector<>;
}

void Viewport::displayGizmo(vml::rectangle<float> sceneRect) {
    using WeakHandle = EntityTransaction::WeakHandle;
    auto entities = selection().entities() | filter([](EntityHandle entity) {
        return entity.has<Transform>();
    }) | ToSmallVector<>;
    if (entities.empty()) {
        gizmo.reset();
        return;
    }
    if (!ImGui::IsAnyItemActive()) {
        viewData.transformBackup = getWeakTransforms(entities);
    }
    auto localTransforms = entities | transform([](EntityHandle entity) {
        return entity.get<Transform>().calculate();
    }) | ToSmallVector<>;
    auto parentTransforms = entities | transform([](EntityHandle entity) {
        if (auto parent = entity.getParent()) {
            return parent.calculateWorldTransformMatrix();
        }
        return float4x4(1);
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
    bool modified =
        gizmo.display(cameraController.camera(), sceneRect, avgWSTransform);
    if (modified) {
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
    if (gizmo.isUsing()) {
        /// Make a copy of the selected entities if shift is held
        if (ImGui::IsKeyDown(ImGuiMod_Shift) &&
            viewData.originalEntitiesBackup.empty())
        {
            viewData.originalEntitiesBackup = entities;
            entities = copyEntities(entities);
            /// Restore original transform for copied entities
            for (auto [index, entity]:
                 viewData.originalEntitiesBackup | enumerate)
            {
                entity.get<Transform>() =
                    std::get<Transform>(viewData.transformBackup[index]);
            }
        }
        /// Undo the copy of the selected entities if shift is released
        if (ImGui::IsKeyReleased(ImGuiMod_Shift) &&
            !viewData.originalEntitiesBackup.empty())
        {
            utl::hashmap<EntityHandle, EntityHandle> copyToOriginal;
            for (auto [copy, original]:
                 zip(entities, viewData.originalEntitiesBackup))
            {
                original.get<Transform>() = copy.get<Transform>();
                copy.deleteEntity();
                copyToOriginal[copy] = original;
            }
            selection().transform([&](EntityHandle copy) {
                BL_ASSERT(copyToOriginal.contains(copy));
                return copyToOriginal[copy];
            });
            entities = viewData.originalEntitiesBackup;
            viewData.originalEntitiesBackup.clear();
        }
    }
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (viewData.originalEntitiesBackup.empty()) {
            auto* TA = new EditEntityComponent{
                entities | ranges::to<utl::small_vector<WeakHandle>>,
                viewData.transformBackup, getWeakTransforms(entities)
            };
            editor().transactionManager().commit(TA);
        }
        else {
            auto newTransforms = entities | transform([](EntityHandle e) {
                return e.get<Transform>();
            }) | ToSmallVector<>;
            ranges::for_each(entities, &EntityHandle::deleteEntity);
            auto postProcess = [newTransforms](EntityHandle copy,
                                               size_t index) {
                copy.get<Transform>() = newTransforms[index];
            };
            auto* scene = viewData.originalEntitiesBackup.front().scene();
            auto* TA =
                new CopyEntitiesTransaction(scene->weak_from_this(),
                                            viewData.originalEntitiesBackup,
                                            selection(),
                                            std::move(postProcess));
            editor().transactionManager().commit(TA);
        }
    }
}

EntityID Viewport::readEntityIDMap() {
    if (!renderer) {
        return {};
    }
    float2 mousePos = window().contentScaleFactor() *
                      screenSpaceToViewSpace(ImGui::GetMousePos());
    return renderer->readEntityIDMap(editor().commandQueue(), mousePos);
}

void Viewport::handleEntitySelection(EntityHandle clickedEntity) {
    BL_ASSERT(ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
              ImGui::IsWindowHovered());
    auto mode = getSelectionMode();
    if (!clickedEntity) {
        editSelection(editor().transactionManager(), selection(),
                      [&selection = selection()] { selection.clear(); });
    }
    else {
        editSelection(editor().transactionManager(), selection(),
                      [=, &selection = selection()] {
            selection.amend(clickedEntity, mode);
        });
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
    mouseState = ::detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
    if (mouseState.held) {
        cameraController.update(editor().time(), editor().input());
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
    auto payload = acceptAssetDragDrop(AssetType::Scene);
    if (!payload) {
        return;
    }
    openScene(*payload);
}

void Viewport::openScene(AssetHandle handle) {
    auto& AM = assetManager();
    auto asset = AM.get(handle);
    auto scene = as<Scene>(asset);
    if (!scene) {
        Logger::Error("Cannot open scene. Asset is not a scene");
        return;
    }
    openScene(scene);
}

void Viewport::openScene(Reference<Scene> scene) {
    using enum AssetRepresentation;
    auto& AM = assetManager();
    AM.makeAvailable(scene->handle(), CPU | GPU);
    auto& sceneSystem = editor().sceneSystem();
    sceneSystem.loadScene(scene);
}

void Viewport::onInput(bloom::InputEvent&) {}

BLOOM_MAKE_TEXT_SERIALIZER(RendererSettings, shadowMapResolution, bloomSettings,
                           postProcessSettings);

YAML::Node Viewport::serialize() const {
    YAML::Node root;
    root["CameraController"] = cameraController;
    root["RendererSettings"] = rendererSettings;
    return root;
}

void Viewport::deserialize(YAML::Node node) {
    cameraController = node["CameraController"].as<ViewportCameraController>();
    rendererSettings = node["RendererSettings"].as<RendererSettings>();
}

Toolbar Viewport::makeToolbar() {
    // clang-format off
    Toolbar toolbar{
        ToolbarDropdownMenu("cog")
            .content([this] { dropdownMenu(); })
            .minWidth(400),
        ToolbarDropdownMenu("bug")
            .content([this] { debuggerMenu(); }),
        ToolbarTextItem()
            .text([this] {
                return scene() ? scene()->name().c_str() : "";
            })
            .font(FontDesc::UIDefault().setWeight(FontWeight::Bold)),
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

void Viewport::displaySceneSettings(Scene& scene) {
    using namespace propertiesView;
    auto physicsParams = scene.physicsParameters();
    beginProperty("Gravity");
    fullWidth();
    if (dragFloat3Labelled("###Gravity", physicsParams.gravity.data(),
                           { .labelStyle = DragFloat3Options::Slim }))
    {
        scene.setPhysicsParameters(physicsParams);
    }
    beginProperty("Render Graph");
    auto rg = scene.renderGraph() ? scene.renderGraph()->shared_from_this() :
                                    Reference<RenderGraph>();
    fullWidth();
    if (assetButton("###RenderGraph", rg,
                    { .style = AssetButtonStyle::Compact,
                      .repr = AssetRepresentation::GPU }))
    {
        scene.setRenderGraph(rg);
        renderGraphDirty = true;
    }
}

void Viewport::displayViewportSettings() {
    using namespace propertiesView;
    beginProperty("Game View");
    ImGui::Checkbox("###game-view", &gameView);
    beginProperty("Field Of View");
    fullWidth();
    if (float fov = cameraController.fieldOfView();
        ImGui::SliderFloat("###field-of-view", &fov, 30, 180))
    {
        cameraController.setFieldOfView(fov);
    }
    beginProperty("Near Clip Plane");
    fullWidth();
    if (float clip = cameraController.nearClipPlane();
        ImGui::SliderFloat("###near-clip-plane", &clip, 0, 1))
    {
        cameraController.setNearClipPlane(clip);
    }
    beginProperty("Camera Speed");
    fullWidth();
    if (float speed = cameraController.speed();
        ImGui::DragFloat("###Camera Speed", &speed))
    {
        cameraController.setSpeed(speed);
    }
}

void Viewport::displayPhysicsSettings() {
    using namespace propertiesView;
    auto& physicsSystem = editor().physicsSystem();
    auto settings = physicsSystem.settings();
    bool modified = false;
    beginProperty("Collision Steps / Frame");
    fullWidth();
    modified |= ImGui::DragInt("###StepsPerFrame",
                               &settings.collisionStepsPerFrame, 0.5, 1,
                               INT_MAX);
    if (modified) {
        physicsSystem.setSettings(settings);
    }
}

static std::string formatResolution(uint2 res) {
    return utl::strcat(res.x, "x", res.y);
}

void Viewport::displayRendererSettings() {
    auto& S = rendererSettings;
    using namespace propertiesView;
    beginProperty("Shadow Map Resolution");
    fullWidth();
    if (ImGui::BeginCombo("###ShadowMapRes",
                          formatResolution(S.shadowMapResolution).c_str()))
    {
        for (int i = 7; i < 14; ++i) {
            uint2 newRes(1 << i);
            if (ImGui::MenuItem(formatResolution(newRes).c_str(), "",
                                newRes == S.shadowMapResolution))
            {
                S.shadowMapResolution = newRes;
            }
        }
        ImGui::EndCombo();
    }
    header("Postprocessing");
    beginProperty("Tone Mapping");
    fullWidth();
    char const* const items[] = { "ACES", "Reinhard" };
    ImGui::Combo("###tone-mapping",
                 (int*)&S.postProcessSettings.toneMappingFunction, items, 2);
    header("Bloom/Veil");
    beginProperty("Enabled");
    bool bloomEnabled = true;
    ImGui::Checkbox("###enabled", &bloomEnabled);
    beginProperty("Physically Correct");
    ImGui::Checkbox("##physically-correct", &S.bloomSettings.physicallyCorrect);
    if (!S.bloomSettings.physicallyCorrect) {
        beginProperty("Intensity");
        fullWidth();
        ImGui::SliderFloat("###intensity", &S.bloomSettings.intensity, 0, 10);
        beginProperty("Threshold");
        fullWidth();
        ImGui::SliderFloat("###threshold", &S.bloomSettings.threshold, 0, 10);
        beginProperty("Soft Knee");
        fullWidth();
        ImGui::SliderFloat("###knee", &S.bloomSettings.knee, 0, 1);
    }
    beginProperty("Clamp");
    fullWidth();
    ImGui::DragFloat("###clamp", &S.bloomSettings.clamp, 0.1, 0, FLT_MAX);
    beginProperty("Diffusion");
    fullWidth();
    ImGui::SliderFloat("###diffusion", &S.bloomSettings.diffusion, 0, 10);
    if (S.bloomSettings.physicallyCorrect) {
        beginProperty("Contribution");
        fullWidth();
        ImGui::SliderFloat("###contribution", &S.bloomSettings.contribution, 0,
                           1);
    }
}

void Viewport::dropdownMenu() {
    using namespace propertiesView;
    ImGui::SetWindowSize({ ImGui::GetWindowSize().x,
                           std::min(ImGui::GetWindowSize().y, 250.0f) },
                         ImGuiCond_Once);
    if (scene() && beginSection("Scene Properties")) {
        displaySceneSettings(*scene());
        endSection();
    }
    if (beginSection("Viewport Settings")) {
        displayViewportSettings();
        endSection();
    }
    if (beginSection("Renderer Settings")) {
        displayRendererSettings();
        endSection();
    }
    if (beginSection("Physics Settings")) {
        displayPhysicsSettings();
        endSection();
    }
}

void Viewport::debuggerMenu() {
    ImGui::MenuItem("Show Render Graph Debugger", "", &showRenderGraphDebugger);
    ImGui::MenuItem("Show Scene Debugger", "", &showSceneDebugger);
}

void Viewport::receiveData(std::any data) {
    auto* scene = std::any_cast<Reference<Scene>>(&data);
    if (!scene) {
        return;
    }
    openScene(*scene);
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
        [&](Geometry2DResource const&) {},
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

static void displayExecutionOrder(RenderGraph const& rg) {
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

static void displayRenderGraphDebugger(RenderGraph const& rg, bool* open) {
    ImGui::Begin("Render Graph Debugger", open);
    if (ImGui::CollapsingHeader("Execution Order",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        displayExecutionOrder(rg);
    }
    if (ImGui::CollapsingHeader("Resource Pool",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (auto* pool = rg.resourcePool()) {
            for (auto* resource: pool->resources()) {
                displayResource(*resource);
            }
        }
        else {
            ImGui::TextDisabled("Resource pool not allocated");
        }
    }
    ImGui::End();
}

static void displaySceneDebugger(Scene const& scene, bool* open) {
    ImGui::Begin("Scene Debugger", open);
    utl::hashset<utl::uuid> persistentIDs;
    utl::hashset<EntityID> notUnique;
    scene.each([&](EntityID ID) {
        if (!persistentIDs.insert(scene.getHandle(ID).persistentID()).second) {
            notUnique.insert(ID);
        }
    });
    scene.each([&, index = 0](EntityID ID) mutable {
        auto entity = scene.getHandle(ID);
        ImGui::PushID(index++);
        if (ImGui::CollapsingHeader(std::string(entity.name()).c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool unique = !notUnique.contains(ID);
            if (!unique) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            }
            ImGui::Text("ID: %s", entity.persistentID().to_string().c_str());
            if (!unique) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::PopID();
    });
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

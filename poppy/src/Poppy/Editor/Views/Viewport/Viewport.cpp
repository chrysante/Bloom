#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/Editor/Views/Viewport/Viewport.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/stack.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/InputEvent.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Graphics/Renderer/ForwardRenderer.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Core/Profile.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/SelectionContext.h"
#include "Poppy/Editor/Views/AssetBrowser.h"
#include "Poppy/Renderer/EditorRenderer.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace mtl::short_types;
using namespace bloom;
using namespace poppy;

POPPY_REGISTER_VIEW(Viewport, "Viewport", {});

Viewport::Viewport(): BasicSceneInspector(this) {
    // clang-format off
    toolbar = {
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

void Viewport::init() {
    setPadding(0);
    toolbar.setStyle({
        .height = 25,
        .buttonAlpha = 1,
        .buttonAlphaHovered = 1,
        .buttonAlphaActive = 1,
    });
    sceneRenderer.setRenderer(&editor().coreSystems().renderer());
    gizmo.setInput(window().input());
    overlays.init(this);
}

void Viewport::shutdown() {}

void Viewport::frame() {
    ImGui::BeginChild("Viewport Child");
    if (scenes().empty()) {
        displayEmptyWithReason("No Active Scene");
    }
    else {
        displayScene();
    }
    /* Display toolbar with padding */ {
        float2 const padding = GImGui->Style.WindowPadding;
        ImGui::SetCursorPos(padding);
        auto* const window = ImGui::GetCurrentWindow();
        auto color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        color.w = 0.5;
        window->DrawList->AddRectFilled(window->Pos,
                                        (float2)window->Pos +
                                            float2{ window->Size.x,
                                                    toolbar.getHeight() +
                                                        2 * padding.y },
                                        ImGui::ColorConvertFloat4ToU32(color));
        toolbar.display(size().x - 2 * padding.x);
    }
    if (!scenes().empty()) {
        auto const wantsInput =
            detectViewportInput(ImGuiButtonFlags_MouseButtonRight);
        viewportHovered = wantsInput.hovered;
        if (wantsInput.held) {
            camera.update(Timestep{ 0, 0.166 }, window().input());
        }
    }
    if (!isSimulating() && GImGui->DragDropActive) {
        float const spacing = 6.5;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::SetCursorPos({ spacing, spacing });
        ImGui::InvisibleButton("viewport-drag-drop-button",
                               ImGui::GetContentRegionAvail() -
                                   ImVec2{ spacing, spacing });
        ImGui::PopItemFlag();
        receiveSceneDragDrop();
    }
    ImGui::EndChild();
}

YAML::Node Viewport::serialize() const {
    YAML::Node node;
    node["Parameters"] = params;
    node["Camera"] = camera;
    return node;
}

void Viewport::deserialize(YAML::Node node) {
    params = node["Parameters"].as<Parameters>();
    camera = node["Camera"].as<ViewportCameraController>();
}

void* Viewport::selectImage() const {
    auto* const fwFramebuffer =
        dynamic_cast<ForwardRendererFramebuffer*>(framebuffer.get());
    using enum Parameters::FramebufferElements;
    if (fwFramebuffer && params.framebufferSlot != Postprocessed) {
        switch (params.framebufferSlot) {
        case Parameters::FramebufferElements::Depth:
            return fwFramebuffer->depth.nativeHandle();
        case Parameters::FramebufferElements::Raw:
            return fwFramebuffer->rawColor.nativeHandle();
        default:
            break;
        }
    }
    if (!gameView) {
        return editorFramebuffer->composed.nativeHandle();
    }
    if (fwFramebuffer) {
        return fwFramebuffer->postProcessed.nativeHandle();
    }
    return nullptr;
}

void Viewport::displayScene() {
    updateFramebuffer();
    if (!framebuffer) {
        return;
    }
    drawScene();
    void* const image = selectImage();
    if (!image) {
        return;
    }
    ImGui::Image(image, ImGui::GetWindowSize());
    if (!gameView) {
        overlays.display();
        gizmo.display(camera.camera(), selection());
    }
}

void Viewport::drawScene() {
    if (!framebuffer) {
        return;
    }
    POPPY_PROFILE(frame);
    camera.applyProjection(framebuffer->size);
    auto& sceneSystem = editor().coreSystems().sceneSystem();
    sceneSystem.applyTransformHierarchy();
    if (gameView) {
        sceneRenderer.draw(sceneSystem.scenes(), camera.camera(), *framebuffer,
                           window().commandQueue());
    }
    else {
        OverlayDrawDescription desc;
        sceneRenderer.drawWithOverlays(sceneSystem.scenes(),
                                       editor().selection(), camera.camera(),
                                       desc, *framebuffer, *editorFramebuffer,
                                       window().commandQueue());
    }
}

void Viewport::updateFramebuffer() {
    auto fbTargetSize = size() * window().contentScaleFactor();
    if (framebuffer && framebuffer->size == fbTargetSize) {
        return;
    }
    Logger::Trace("Resizing framebuffer to ", fbTargetSize);
    framebuffer = sceneRenderer.renderer().createFramebuffer(fbTargetSize);
    auto* renderer = dynamic_cast<EditorRenderer*>(&sceneRenderer.renderer());
    if (renderer) {
        editorFramebuffer = renderer->createEditorFramebuffer(fbTargetSize);
    }
}

void Viewport::onInput(bloom::InputEvent& event) {
    event.dispatch<bloom::InputEventType::leftMouseDown>(
        [&](bloom::MouseEvent const& e) {
        if (!viewportHovered || gizmo.isHovered() || gameView) {
            return false;
        }
        auto const locationInView = windowSpaceToViewSpace(e.locationInWindow);
        if (auto const entity = overlays.hitTest(locationInView)) {
            selection().select(entity);
            return true;
        }
        if (auto const entity = readEntityID(locationInView)) {
            selection().select(entity);
            return true;
        }
        selection().clear();
        return true;
    });
    event.dispatch<bloom::InputEventType::keyDown>(
        [&](bloom::KeyEvent const& e) {
        using bloom::Key;
        switch (e.key) {
        case Key::Tab:
            gizmo.cycleSpace();
            break;
        case Key::_1:
            gizmo.setOperation(Gizmo::Operation::Translate);
            break;
        case Key::_2:
            gizmo.setOperation(Gizmo::Operation::Rotate);
            break;
        case Key::_3:
            gizmo.setOperation(Gizmo::Operation::Scale);
            break;
        case Key::Escape:
            restore();
        case Key::G:
            gameView ^= true;
        default:
            break;
        }
    });
}

void Viewport::debugPanel() {
    auto matrix = [](float4x4 const& m) {
        withFont(Font::UIDefault().setMonospaced(true), [&] {
            for (int i = 0; i < 4; ++i) {
                ImGui::TextUnformatted(utl::strcat(m.row(i)).data());
            }
        });
    };
    ImGui::Begin("Viewport Debug");
    float4x4 view = camera.camera().view();
    float4x4 proj = camera.camera().projection();
    withFont(Font::UIDefault().setWeight(FontWeight::semibold),
             [&] { ImGui::Text("View Matrix:"); });
    matrix(view);
    withFont(Font::UIDefault().setWeight(FontWeight::semibold),
             [&] { ImGui::Text("Projection Matrix:"); });
    matrix(proj);
    ImGui::End();
}

namespace poppy {

/// This function must be in `namespace poppy` because it is a customization
/// point for some templates
static std::string toString(Viewport::Parameters::FramebufferElements value) {
    return std::array{ "Depth", "Raw", "Postprocessed" }[(std::size_t)value];
}

} // namespace poppy

void Viewport::dropdownMenu() {
    using namespace propertiesView;
    ImGui::SetWindowSize({ ImGui::GetWindowSize().x,
                           std::min(ImGui::GetWindowSize().y, 250.0f) });
    header("Viewport Settings");
    if (beginSection()) {
        beginProperty("Game View");
        ImGui::Checkbox("##game-view", &gameView);

        beginProperty("Field Of View");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (float fov = camera.fieldOfView();
            ImGui::SliderFloat("##field-of-view", &fov, 30, 180))
        {
            camera.setFieldOfView(fov);
        }

        beginProperty("Near Clip Plane");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (float clip = camera.nearClipPlane();
            ImGui::SliderFloat("##near-clip-plane", &clip, 0, 1))
        {
            camera.setNearClipPlane(clip);
        }

        beginProperty("Visualize Shadow Cascades");
        if (ImGui::Checkbox("##viz-shadow-cascades",
                            &drawOptions.visualizeShadowCascades) &&
            !drawOptions.lightVizEntity)
        {
            /// Search for first dir light entity
            for (auto scene: scenes()) {
                for (auto&& [entity, light]:
                     scene->view<DirectionalLightComponent const>().each())
                {
                    if (light.light.castsShadows) {
                        drawOptions.lightVizEntity = scene->getHandle(entity);
                        break;
                    }
                }
            }
        }
        if (drawOptions.visualizeShadowCascades) {
            auto lightEntity = drawOptions.lightVizEntity;
            auto* scene = &lightEntity.scene();
            char const* preview = nullptr;
            if (!lightEntity || getLightType(lightEntity) == LightType::none ||
                !scene->getComponent<DirectionalLightComponent>(lightEntity)
                     .light.castsShadows)
            {
                preview = "Select Shadow Caster";
            }
            else {
                preview =
                    scene->getComponent<TagComponent>(lightEntity).name.data();
            }
            beginProperty("Shadow Caster");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::BeginCombo("##shadow-caster", preview,
                                  ImGuiComboFlags_NoArrowButton))
            {
                if (scene) {
                    for (auto&& [entity, light, tag]:
                         scene
                             ->view<DirectionalLightComponent const,
                                    TagComponent const>()
                             .each())
                    {
                        if (light.light.castsShadows &&
                            ImGui::Selectable(tag.name.data()))
                        {
                            drawOptions.lightVizEntity =
                                ConstEntityHandle(entity, scene);
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }
        endSection();
    }

    header("Framebuffer");
    if (beginSection()) {
        beginProperty("Slot");
        if (ImGui::BeginCombo("##framebuffer-slot",
                              toString(params.framebufferSlot).data(),
                              ImGuiComboFlags_NoArrowButton))
        {
            enumCombo(params.framebufferSlot);
            ImGui::EndCombo();
        }
        endSection();
    }
}

bloom::EntityHandle Viewport::readEntityID(mtl::float2 mousePosition) {
    Logger::Warn("Mouse picking is not implemented");
    return {};
    /// This doesn't work right now because we don't render the entity ID
    /// framebuffer
#if 0
    static_assert(
        std::is_same_v<std::underlying_type_t<entt::entity>, std::uint32_t>);
    if (scenes().empty()) {
        return {};
    }
    if (mousePosition.x < 0 || mousePosition.y < 0) {
        return {};
    }
    auto const viewSize = this->size();
    if (mousePosition.x >= viewSize.x || mousePosition.y >= viewSize.y) {
        return {};
    }
    auto* entityTexture = (MTL::Texture*)framebuffer.entityID.nativeHandle();
    auto* mtlRenderContext = utl::down_cast<bloom::MetalRenderContext*>(
        renderer->getRenderContext());
    auto* commandBuffer = mtlRenderContext->commandQueue()->commandBuffer();
    auto* encoder = commandBuffer->blitCommandEncoder();
    encoder->synchronizeTexture(entityTexture, 0, 0);
    encoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();

    std::uint32_t id = -1;
    auto const region =
        MTL::Region(mousePosition.x * 2, mousePosition.y * 2, 1, 1);
    entityTexture->getBytes(&id, 4, region, 0);

    return bloom::EntityID((entt::entity)id);
#endif
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

mtl::float3 Viewport::worldSpaceToViewSpace(mtl::float3 const positionWS) {
    auto const viewProj = camera.camera().viewProjection();
    auto const tmp = viewProj * mtl::float4(positionWS, 1);
    auto const ndc = tmp.xyz / tmp.w;

    auto posVS = ndc.xy;

    posVS = (posVS + 1) / 2;
    posVS.y = 1 - posVS.y;
    posVS *= this->size();
    return { posVS, ndc.z };
}

mtl::float3 Viewport::worldSpaceToWindowSpace(mtl::float3 position) {
    auto const vs = worldSpaceToViewSpace(position);
    return { viewSpaceToWindowSpace(vs.xy), vs.z };
}

mtl::float2 Viewport::worldSpaceDirToViewSpace(mtl::float3 const positionWS) {
    auto const viewProj = camera.camera().viewProjection();
    auto const tmp = viewProj * mtl::float4(positionWS, 0);
    auto const ndc = tmp.xyz / tmp.w;

    auto dirVS = ndc.xy;

    dirVS.y = 1 - dirVS.y;
    dirVS *= this->size();
    return dirVS;
}

mtl::float2 Viewport::worldSpaceDirToWindowSpace(mtl::float3 direction) {
    return worldSpaceDirToViewSpace(direction);
}

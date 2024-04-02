#include "Poppy/Editor/Views/RenderGraphView.h"

#include <imgui.h>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/Editor.h"

using namespace poppy;
using namespace bloom;
using namespace vml;

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

static rg::RenderGraph* getGlobalRenderGraph(HardwareDevice* device) {
    static auto g = rg::createDefaultRenderGraph(device);
    return g.get();
}

RenderGraphView::RenderGraphView(): BasicSceneInspector(this) {}

void RenderGraphView::init() {
    using namespace nodeEditor;

    static constexpr float4 InputColor = { 0.4, 0.7, 0.2, 1.0 };
    static constexpr float4 PassColor = { 0.3, 0.0, 0.8, 1.0 };
    static constexpr float4 OutputColor = { 1.0, 0.0, 0.2, 1.0 };

    nodeEditor.setDelegate({ .backgroundPopupMenu = [] {
        if (ImGui::BeginMenu("Inputs")) {
            if (ImGui::MenuItem("Scene Geometry")) {
            }
            if (ImGui::MenuItem("Scene Lights")) {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Outputs")) {
            if (ImGui::MenuItem("Swapchain Present")) {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Passes")) {
            if (ImGui::MenuItem("Shadow Pass")) {
            }
            ImGui::EndMenu();
        }
    } });

    // clang-format off
    nodeEditor.addNode({
        .name = "Scene Geometry",
        .position = { 100, 100 },
        .minSize = { 80, 0 },
        .inputs = {},
        .outputs = { { .name = "Data" } },
        .color = InputColor
    });
    nodeEditor.addNode({
        .name = "Scene Lights",
        .position = { 100, 200 },
        .minSize = { 80, 0 },
        .inputs = {},
        .outputs = { { .name = "Data" } },
        .color = InputColor
    });
    nodeEditor.addNode({
        .name = "Shadow Pass",
        .position = { 200, 400 },
        .minSize = { 150, 100 },
        .inputs = {
            { .name = "Geometry" },
            { .name = "Lights" }
        },
        .outputs = { { .name = "Shadow Map" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Main Pass",
        .position = { 200, 600 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Geometry" }, { .name = "Lights" }, { .name = "Shadow Map" } },
        .outputs = { { .name = "Raw Color" }, { .name = "Depth Buffer" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Bloom Pass",
        .position = { 200, 700 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Raw Color" } },
        .outputs = { { .name = "Glow Color" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Post Process Pass",
        .position = { 200, 800 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Raw Color" }, { .name = "Depth" }, { .name = "Glow Color" } },
        .outputs = { { .name = "Final Color" } },
        .color = PassColor
    });
    nodeEditor.addNode({
        .name = "Swapchain Present",
        .position = { 200, 800 },
        .minSize = { 100, 80 },
        .inputs = { { .name = "Color" } },
        .color = OutputColor,
    });
    // clang-format on

    /// For now!!
    renderGraph = getGlobalRenderGraph(&editor().device());
}

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

template <typename LightComponent>
static void submitLights(rg::LightsResource& lights, Scene const& scene,
                         auto&& lightModifier) {
    auto view =
        scene.view<TransformMatrixComponent const, LightComponent const>();
    view.each([&](auto /* ID */, TransformMatrixComponent const& transform,
                  LightComponent light) {
        lightModifier(transform, light);
        lights.add(light.light);
    });
}

template <typename Light>
static void submitLights(rg::LightsResource& lights, Scene const& scene) {
    submitLights<Light>(lights, scene, [](auto&...) {});
}

static void submitScene(HardwareDevice& device, SceneSystem& sceneSystem,
                        rg::GeometryResource& geometry,
                        rg::LightsResource& lights) {
    geometry.clear();
    lights.clear();
    auto* scene = sceneSystem.scenes().front();
    std::unique_lock lock = sceneSystem.lock();
    sceneSystem.applyTransformHierarchy();
    auto view = scene->view<TransformMatrixComponent const,
                            MeshRendererComponent const>();
    view.each([&](auto /* ID */, TransformMatrixComponent const& transform,
                  MeshRendererComponent const& meshRenderer) {
        if (!meshRenderer.mesh || !meshRenderer.materialInstance ||
            !meshRenderer.materialInstance->material())
        {
            return;
        }
        geometry.add(meshRenderer.mesh->getRenderer(),
                     meshRenderer.materialInstance, transform.matrix);
    });
    submitLights<PointLightComponent>(lights, *scene,
                                      [](auto& transform, auto& light) {
        light.light.position = transform.matrix.column(3).xyz;
    });
    submitLights<SpotLightComponent>(lights, *scene,
                                     [](auto& transform, auto& light) {
        light.light.position = transform.matrix.column(3).xyz;
        light.light.direction =
            vml::normalize((transform.matrix * vml::float4{ 1, 0, 0, 0 }).xyz);
    });
    submitLights<DirectionalLightComponent>(lights, *scene,
                                            [](auto& transform, auto& light) {
        light.light.direction =
            vml::normalize((transform.matrix * vml::float4{ 0, 0, 1, 0 }).xyz);
    });
    submitLights<SkyLightComponent>(lights, *scene);
    lights.prepare(device);
    geometry.prepare(device);
}

void RenderGraphView::frame() {
    if (scenes().empty()) {
        displayEmptyWithReason("No Active Scene");
        return;
    }
    auto windows = editor().getWindows();
    if (windows.empty()) {
        return;
    }
    vml::float2 viewportSize = ImGui::GetContentRegionAvail();
    rg::FrameParameters frameParams = { .screenSize = viewportSize };
    renderGraph->allocateResources(frameParams);
    auto& device = editor().device();
    auto* window = windows.front();
    auto* mainPass = renderGraph->findPass("MainPass");
    auto* geometry = cast<rg::GeometryResource*>(
        renderGraph->findInputSource("Geometry")->resource());
    BL_ASSERT(geometry);
    auto* lights = cast<rg::LightsResource*>(
        renderGraph->findInputSource("Lights")->resource());
    BL_ASSERT(lights);
    auto* paramBuffer = cast<rg::BufferResource*>(
        renderGraph->findInputSource("Parameters")->resource());
    BL_ASSERT(paramBuffer && paramBuffer->handle());
    Camera camera;
    camera.setProjection(vml::constants<>::pi / 2, viewportSize);
    camera.setTransform(/* position: */ { -1429, 431, 887 },
                        /* front: */ { 1, 0, 0 },
                        /* up: */ { 0, 0, 1 });
    SceneRenderParameters sceneParams = { .camera = vml::transpose(
                                              camera.viewProjection()),
                                          .cameraPosition = camera.position(),
                                          .screenSize = viewportSize,
                                          .screenResolution = viewportSize };
    device.fillManagedBuffer(paramBuffer->handle(), &sceneParams,
                             sizeof sceneParams);
    submitScene(device, editor().coreSystems().sceneSystem(), *geometry,
                *lights);
    renderGraph->execute(window->commandQueue());
    auto* output = cast<rg::TextureResource*>(
        mainPass->findSource("RawColor")->resource());
    BL_ASSERT(output && output->handle());
    ImGui::Image(output->handle().nativeHandle(),
                 ImGui::GetContentRegionAvail());
}

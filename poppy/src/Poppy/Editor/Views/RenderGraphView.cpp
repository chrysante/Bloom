#include "Poppy/Editor/Views/RenderGraphView.h"

#include <imgui.h>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Graphics/RenderGraph/RenderGraph.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"
#include "Bloom/Graphics/RenderGraph/SceneRenderer.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Scene.h"
#include "Bloom/Shader/PostProcessing.h"
#include "Poppy/Editor/Editor.h"

using namespace poppy;
using namespace bloom;
using namespace vml;

POPPY_REGISTER_VIEW(RenderGraphView, "Render Graph View", {})

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
    renderGraph = rg::getGlobalRenderGraph(&editor().device());
}

void RenderGraphView::onInput(InputEvent& event) { nodeEditor.onInput(event); }

static rg::SceneRenderer& getSceneRenderer(rg::RenderGraph* renderGraph) {
    static rg::SceneRenderer instance(renderGraph);
    return instance;
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
    Camera camera;
    camera.setProjection(vml::constants<>::pi / 2, viewportSize);
    camera.setTransform(/* position: */ { -1429, 431, 887 },
                        /* front: */ { 1, 0, 0 },
                        /* up: */ { 0, 0, 1 });

    auto& renderer = getSceneRenderer(renderGraph);
    renderer.begin(frameParams, camera);
    auto& sceneSystem = editor().coreSystems().sceneSystem();
    std::unique_lock lock = sceneSystem.lock();
    sceneSystem.applyTransformHierarchy();
    for (auto* scene: sceneSystem.scenes()) {
        renderer.submit(*scene);
    }
    lock.unlock();
    renderer.draw(windows.front()->commandQueue());
    auto* postProcessPass = renderGraph->findPass("PostProcess");
    auto* output = cast<rg::TextureResource*>(
        postProcessPass->findSource("Output")->resource());
    ImGui::Image(output->handle().nativeHandle(),
                 ImGui::GetContentRegionAvail());
}

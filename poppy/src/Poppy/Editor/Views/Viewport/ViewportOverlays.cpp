#include "Poppy/Editor/Views/Viewport/ViewportOverlays.h"

#include <imgui.h>

#include "Bloom/Graphics/Camera.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/SelectionContext.h"
#include "Poppy/Editor/Views/Viewport/Viewport.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

static std::uint32_t toUCol32(mtl::float4 color) {
    return ImGui::ColorConvertFloat4ToU32(color);
}

static std::uint32_t toUCol32(mtl::float3 color) {
    return toUCol32(mtl::float4{ color, 1 });
}

static void drawArrow(mtl::float2 from, mtl::float2 to, float headSize,
                      mtl::float4 color, float thickness) {
    float2 const p0 = from;
    float2 const line = to - from;

    float const angle = std::acos(mtl::dot(mtl::normalize(line), float2(1, 0)));
    float2x2 const arrowRot = { std::cos(angle), -std::sin(angle),
                                std::sin(angle), std::cos(angle) };
    float2 const t0 = arrowRot * headSize * float2{ 0, 0 };
    float2 const t1 = arrowRot * headSize * float2{ -std::sqrt(3) / 2, -0.5 };
    float2 const t2 = arrowRot * headSize * float2{ -std::sqrt(3) / 2, 0.5 };

    auto* const drawList = ImGui::GetWindowDrawList();

    std::array<float2, 5> const positions = {
        p0, p0 + (mtl::norm(line) - headSize) * mtl::normalize(line),
        p0 + line + t0, p0 + line + t1, p0 + line + t2
    };

    drawList->AddLine(positions[0], positions[1], toUCol32(color), thickness);
    drawList->AddTriangle(positions[2], positions[3], positions[4],
                          toUCol32(color), thickness);
}

void ViewportOverlays::init(Viewport* viewport) { this->viewport = viewport; }

void ViewportOverlays::display() {
    drawLightOverlays<PointLightComponent>();
    drawLightOverlays<SpotLightComponent>();
    drawLightOverlays<DirectionalLightComponent>();
    drawLightOverlays<SkyLightComponent>();
}

bloom::EntityHandle ViewportOverlays::hitTest(mtl::float2 positionInView) {
    if (auto result = hitTestLightOverlays<PointLightComponent>(positionInView))
    {
        return result;
    }
    if (auto result = hitTestLightOverlays<SpotLightComponent>(positionInView))
    {
        return result;
    }
    if (auto result =
            hitTestLightOverlays<DirectionalLightComponent>(positionInView))
    {
        return result;
    }
    if (auto result = hitTestLightOverlays<SkyLightComponent>(positionInView)) {
        return result;
    }
    return {};
}

template <typename Light>
void ViewportOverlays::drawLightOverlays() {
    auto const& selection = viewport->editor().selection();
    for (auto&& scene: viewport->editor().coreSystems().sceneSystem().scenes())
    {
        auto view = scene->view<TransformMatrixComponent const, Light const>();
        view.each([&](auto const id, TransformMatrixComponent const& transform,
                      Light const& light) {
            auto const positionVS =
                viewport->worldSpaceToViewSpace(transform.matrix.column(3).xyz);
            if (positionVS.z < 0 || positionVS.z > 1) {
                return;
            }
            auto const positionInWindow =
                viewport->viewSpaceToWindowSpace(positionVS.xy);
            auto const entity = scene->getHandle(id);
            drawOneLightOverlay(entity, positionInWindow,
                                selection.isSelected(entity),
                                Transform::fromMatrix(transform.matrix),
                                light.light);
        });
    }
}

template <>
void ViewportOverlays::drawOneLightOverlay(bloom::EntityHandle entity,
                                           mtl::float2 positionInWindow,
                                           bool selected,
                                           Transform const& transform,
                                           PointLight const& light) {
    drawPointLightIcon(positionInWindow, light.common.color);
}

template <>
void ViewportOverlays::drawOneLightOverlay(bloom::EntityHandle entity,
                                           mtl::float2 positionInWindow,
                                           bool selected,
                                           Transform const& transform,
                                           SpotLight const& s) {
    drawSpotLightIcon(positionInWindow, s.common.color);
    if (selected) {
        drawSpotlightVizWS(entity, s.radius,
                           (s.innerCutoff + s.outerCutoff) / 2,
                           (s.common.color + mtl::colors<float3>::white) / 2);
    }
}

template <>
void ViewportOverlays::drawOneLightOverlay(bloom::EntityHandle entity,
                                           mtl::float2 positionInWindow,
                                           bool selected,
                                           Transform const& transform,
                                           DirectionalLight const& light) {
    float3 const lightDirWS = mtl::rotate({ 0, 0, 1 }, transform.orientation);
    drawDirectionalLightIcon(positionInWindow, lightDirWS, light.common.color);
}

template <>
void ViewportOverlays::drawOneLightOverlay(bloom::EntityHandle entity,
                                           mtl::float2 positionInWindow,
                                           bool selected,
                                           Transform const& transform,
                                           SkyLight const& light) {
    drawSkyLightIcon(positionInWindow, light.common.color);
}

void ViewportOverlays::drawPointLightIcon(mtl::float2 position,
                                          mtl::float3 color) {
    float const size = 20;
    auto* const drawList = ImGui::GetWindowDrawList();
    drawList->AddCircle(position, size, toUCol32({ 0, 0, 0, 0.5 }), 0, 4);
    drawList->AddCircle(position, size, toUCol32(color), 0, 2);
}

void ViewportOverlays::drawSpotLightIcon(mtl::float2 position,
                                         mtl::float3 color) {
    float const radius = 25;
    float2 const v0 = float2(0, 1) * radius + position;
    float const angle1 = mtl::constants<>::pi * (0.5 + 2. / 3.);
    float2 const v1 =
        float2(std::cos(angle1), std::sin(angle1)) * radius + position;
    float const angle2 = mtl::constants<>::pi * (0.5 + 4. / 3.);
    float2 const v2 =
        float2(std::cos(angle2), std::sin(angle2)) * radius + position;

    auto* const drawList = ImGui::GetWindowDrawList();
    drawList->AddTriangle(v0, v1, v2, toUCol32({ 0, 0, 0, 0.5 }), 4);
    drawList->AddTriangle(v0, v1, v2, toUCol32(color), 2);
}

void ViewportOverlays::drawDirectionalLightIcon(mtl::float2 position,
                                                mtl::float3 directionWS,
                                                mtl::float3 color) {
    float const size = 18;

    float2 const line = float2(-1, 2) * size;

    auto doDraw = [&](float4 color, float thickness) {
        for (auto offset: { -1, 0, 1 }) {
            float2 const p = position + float2(offset + 0.5, -1) * size;
            drawArrow(p, p + line, size * 0.6, color, thickness);
        }
    };
    doDraw(float4(0, 0, 0, 0.5), 4);
    doDraw(float4(color, 1), 2);
}

void ViewportOverlays::drawSkyLightIcon(mtl::float2 position,
                                        mtl::float3 color) {
    float const size = 20;
    float const lineSize = size;

    auto doDraw = [&](float4 color, float thickness) {
        std::array<ImVec2, 9> points{};
        for (int i = 0; i < 9; ++i) {
            float angle = mtl::constants<>::pi * (0.5 + (i - 4) / 4.0 * 0.25);

            float2 const dir = float2(std::cos(-angle), std::sin(-angle));
            float2 const from = position + (float2(0, 1) + dir * 2) * size;
            points[i] = from;
            if (i % 2) {
                continue;
            }
            float2 const line = -dir * lineSize;
            drawArrow(from, from + line, 0.3 * lineSize, color, thickness);
        }
        ImGui::GetWindowDrawList()->AddPolyline(points.data(), 9,
                                                toUCol32(color),
                                                ImDrawFlags_None, 2);
    };
    doDraw(float4(0, 0, 0, 0.5), 4);
    doDraw(float4(color, 1), 2);
}

void ViewportOverlays::drawSpotlightVizWS(bloom::EntityHandle entity,
                                          float radius, float angle,
                                          mtl::float3 color) {
    auto* const drawList = ImGui::GetWindowDrawList();
    int const segments = 32;
    utl::small_vector<float3, 32> circlePoints1;
    for (int i = 0; i < segments; ++i) {
        float const tau = mtl::constants<>::pi * 2;
        float const angle = i * tau / segments;
        circlePoints1.push_back({ 0, cos(angle), sin(angle) });
    }
    float const theta = angle;
    float const r1 = radius;
    float const r2 = radius + 50;
    float const rho1 = r1 * sin(theta);
    float const rho2 = r2 * sin(theta);
    float const sigma1 = r1 * cos(theta);
    float const sigma2 = r2 * cos(theta);

    float4x4 const transform =
        entity.scene().calculateTransformRelativeToWorld(entity);

    auto circlePoints2 = circlePoints1;
    for (auto& p: circlePoints1) {
        p.x = sigma1;
        p.y *= rho1;
        p.z *= rho1;
        p = (transform * float4(p, 1)).xyz;
    }
    for (auto& p: circlePoints2) {
        p.x = sigma2;
        p.y *= rho2;
        p.z *= rho2;
        p = (transform * float4(p, 1)).xyz;
    }

    utl::small_vector<float2, 32> pointsWindowSpace1;
    std::transform(circlePoints1.begin(), circlePoints1.end(),
                   std::back_inserter(pointsWindowSpace1), [this](float3 x) {
        return viewport->worldSpaceToWindowSpace(x).xy;
    });
    utl::small_vector<float2, 32> pointsWindowSpace2;
    std::transform(circlePoints2.begin(), circlePoints2.end(),
                   std::back_inserter(pointsWindowSpace2), [this](float3 x) {
        return viewport->worldSpaceToWindowSpace(x).xy;
    });

    for (int i = 0; i < segments; i += 4) {
        drawList->AddLine(pointsWindowSpace1[i], pointsWindowSpace2[i],
                          toUCol32(color));
    }
    for (int i = 0; i < segments; ++i) {
        drawList->AddLine(pointsWindowSpace1[i],
                          pointsWindowSpace1[(i + 1) % segments],
                          toUCol32(color));
        drawList->AddLine(pointsWindowSpace2[i],
                          pointsWindowSpace2[(i + 1) % segments],
                          toUCol32(color));
    }
}

template <typename Light>
EntityHandle ViewportOverlays::hitTestLightOverlays(
    mtl::float2 hitPositionInView) {
    for (auto&& scene: viewport->editor().coreSystems().sceneSystem().scenes())
    {
        auto componentView =
            scene->view<TransformMatrixComponent const, Light const>();
        for (auto&& [id, transform, light]: componentView.each()) {
            auto const entityPositionInView =
                viewport->worldSpaceToViewSpace(transform.matrix.column(3).xyz);
            if (entityPositionInView.z < 0 || entityPositionInView.z > 1) {
                continue;
            }

            auto const entity = scene->getHandle(id);

            bool const result =
                hitTestOneLightOverlay<Light>(entityPositionInView.xy,
                                              hitPositionInView);
            if (result) {
                return entity;
            }
        }
    }
    return {};
}

template <typename Light>
bool ViewportOverlays::hitTestOneLightOverlay(
    mtl::float2 entityPositionInWindow, mtl::float2 hitPositionInWindow) {
    return mtl::distance(entityPositionInWindow, hitPositionInWindow) < 30;
}

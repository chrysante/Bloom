#include "Poppy/Views/ViewportExt.h"

#include <imgui.h>
#include <utl/functional.hpp>

#include "Bloom/Graphics/Camera.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/EntitySelectionManager.h"
#include "Poppy/Views/Viewport/Viewport.h"

using namespace bloom;
using namespace vml;
using namespace poppy;

std::string poppy::toString(Projection proj) {
    return std::array{ "Perspective", "Orthogonal" }[(size_t)proj];
}

void ViewportCameraController::update(Timestep time, Input const& input) {

    data.angleLR = utl::mod(data.angleLR - input.mouseOffset().x / 180.,
                            constants<>::pi * 2);
    data.angleUD = std::clamp(data.angleUD + input.mouseOffset().y / 180.f,
                              0.01f, constants<float>::pi - 0.01f);
    float offset = data.speed * time.delta;
    if (input.keyDown(Key::LeftShift)) {
        offset *= 3;
    }
    if (input.keyDown(Key::A)) {
        data.position -= normalize(cross(front(), up())) * offset;
    }
    if (input.keyDown(Key::D)) {
        data.position += normalize(cross(front(), up())) * offset;
    }
    if (input.keyDown(Key::W)) {
        data.position += front() * offset;
    }
    if (input.keyDown(Key::S)) {
        data.position -= front() * offset;
    }
    if (input.keyDown(Key::Q)) {
        data.position -= up() * offset;
    }
    if (input.keyDown(Key::E)) {
        data.position += up() * offset;
    }
    applyTransform();
}

void ViewportCameraController::applyTransform() {
    cam.setTransform(data.position, front());
}

void ViewportCameraController::applyProjection(float2 screenSize) {
    if (data.projection == Projection::Perspective) {
        cam.setProjection(to_radians(data.fieldOfView), screenSize,
                          data.nearClip);
    }
    else {
        float width = 1000;
        float aspect = screenSize.y / screenSize.x;
        float height = width * aspect;
        cam.setProjectionOrtho(-width / 2, width / 2, -height / 2, height / 2,
                               0, 5000);
    }
}

float3 ViewportCameraController::front() const {
    using std::cos;
    using std::sin;
    return { sin(data.angleUD) * cos(data.angleLR),
             sin(data.angleUD) * sin(data.angleLR), cos(data.angleUD) };
}

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewportCameraController::Data, angleLR,
                           angleUD, speed, position, projection, fieldOfView,
                           nearClip);

YAML::Node ViewportCameraController::serialize() const {
    return YAML::Node(data);
}

bool ViewportCameraController::deserialize(YAML::Node const& node) {
    data = node.as<Data>();
    applyTransform();
    return true;
}

static std::uint32_t toUCol32(float4 color) {
    return ImGui::ColorConvertFloat4ToU32(color);
}

static std::uint32_t toUCol32(float3 color, float alpha = 1.0f) {
    return toUCol32(float4{ color, alpha });
}

static void drawArrow(float2 from, float2 to, float headSize, float4 color,
                      float thickness) {
    float2 const p0 = from;
    float2 const line = to - from;

    float const angle = std::acos(dot(normalize(line), float2(1, 0)));
    float2x2 const arrowRot = { std::cos(angle), -std::sin(angle),
                                std::sin(angle), std::cos(angle) };
    float2 const t0 = arrowRot * headSize * float2{ 0, 0 };
    float2 const t1 = arrowRot * headSize * float2{ -std::sqrt(3) / 2, -0.5 };
    float2 const t2 = arrowRot * headSize * float2{ -std::sqrt(3) / 2, 0.5 };

    auto* const drawList = ImGui::GetWindowDrawList();

    std::array<float2, 5> const positions = {
        p0, p0 + (norm(line) - headSize) * normalize(line), p0 + line + t0,
        p0 + line + t1, p0 + line + t2
    };

    drawList->AddLine(positions[0], positions[1], toUCol32(color), thickness);
    drawList->AddTriangle(positions[2], positions[3], positions[4],
                          toUCol32(color), thickness);
}

void ViewportOverlays::drawOneLightOverlay(EntityHandle entity,
                                           float2 positionOnScreen,
                                           bool selected, Transform const&,
                                           SpotLight const& s) {
    drawSpotLightIcon(positionOnScreen, s.common.color);
    if (selected) {
        drawSpotlightVizWS(entity, s.radius,
                           (s.innerCutoff + s.outerCutoff) / 2,
                           (s.common.color + colors<float3>::white) / 2);
    }
}

void ViewportOverlays::drawOneLightOverlay(EntityHandle,
                                           float2 positionOnScreen,
                                           bool /* selected */,
                                           Transform const& transform,
                                           DirectionalLight const& light) {
    float3 const lightDirWS = rotate({ 0, 0, 1 }, transform.orientation);
    drawDirectionalLightIcon(positionOnScreen, lightDirWS, light.common.color);
}

void ViewportOverlays::drawOneLightOverlay(EntityHandle,
                                           float2 positionOnScreen,
                                           bool /* selected */,
                                           Transform const&,
                                           SkyLight const& light) {
    drawSkyLightIcon(positionOnScreen, light.common.color);
}

void ViewportOverlays::drawSpotLightIcon(float2 position, float3 color) {
    float const radius = 25;
    float2 const v0 = float2(0, 1) * radius + position;
    float const angle1 = constants<>::pi * (0.5 + 2. / 3.);
    float2 const v1 =
        float2(std::cos(angle1), std::sin(angle1)) * radius + position;
    float const angle2 = constants<>::pi * (0.5 + 4. / 3.);
    float2 const v2 =
        float2(std::cos(angle2), std::sin(angle2)) * radius + position;

    auto* const drawList = ImGui::GetWindowDrawList();
    drawList->AddTriangle(v0, v1, v2, toUCol32({ 0, 0, 0, 0.5 }), 4);
    drawList->AddTriangle(v0, v1, v2, toUCol32(color), 2);
}

void ViewportOverlays::drawDirectionalLightIcon(float2 position,
                                                float3 /* directionWS */,
                                                float3 color) {
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

void ViewportOverlays::drawSkyLightIcon(float2 position, float3 color) {
    float const size = 20;
    float const lineSize = size;

    auto doDraw = [&](float4 color, float thickness) {
        std::array<ImVec2, 9> points{};
        for (int i = 0; i < 9; ++i) {
            float angle = constants<>::pi * (0.5 + (i - 4) / 4.0 * 0.25);

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

void ViewportOverlays::drawSpotlightVizWS(EntityHandle entity, float radius,
                                          float angle, float3 color) {
    auto* const drawList = ImGui::GetWindowDrawList();
    int const segments = 32;
    utl::small_vector<float3, 32> circlePoints1;
    for (int i = 0; i < segments; ++i) {
        float const tau = constants<>::pi * 2;
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

    utl::small_vector<float2, 32> pointsScreenSpace1;
    std::transform(circlePoints1.begin(), circlePoints1.end(),
                   std::back_inserter(pointsScreenSpace1), [this](float3 x) {
        return viewport->worldSpaceToScreenSpace(x).xy;
    });
    utl::small_vector<float2, 32> pointsScreenSpace2;
    std::transform(circlePoints2.begin(), circlePoints2.end(),
                   std::back_inserter(pointsScreenSpace2), [this](float3 x) {
        return viewport->worldSpaceToScreenSpace(x).xy;
    });

    for (int i = 0; i < segments; i += 4) {
        drawList->AddLine(pointsScreenSpace1[i], pointsScreenSpace2[i],
                          toUCol32(color));
    }
    for (int i = 0; i < segments; ++i) {
        drawList->AddLine(pointsScreenSpace1[i],
                          pointsScreenSpace1[(i + 1) % segments],
                          toUCol32(color));
        drawList->AddLine(pointsScreenSpace2[i],
                          pointsScreenSpace2[(i + 1) % segments],
                          toUCol32(color));
    }
}

void ViewportOverlays::init(Viewport* viewport) { this->viewport = viewport; }

void ViewportOverlays::display() {
    drawLightOverlays<PointLightComponent>();
    drawLightOverlays<SpotLightComponent>();
    drawLightOverlays<DirectionalLightComponent>();
    drawLightOverlays<SkyLightComponent>();
}

template <typename>
void ViewportOverlays::drawLightOverlays() {}

bloom::EntityHandle ViewportOverlays::hitTest(vml::float2 positionInView) {
    hitTestLightOverlays<PointLightComponent>(positionInView);
    hitTestLightOverlays<SpotLightComponent>(positionInView);
    hitTestLightOverlays<DirectionalLightComponent>(positionInView);
    hitTestLightOverlays<SkyLightComponent>(positionInView);
    return {};
}

template <typename Light>
EntityHandle ViewportOverlays::hitTestLightOverlays(float2 hitPositionInView) {
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
bool ViewportOverlays::hitTestOneLightOverlay(float2 entityPositionInWindow,
                                              float2 hitPositionInWindow) {
    return distance(entityPositionInWindow, hitPositionInWindow) < 30;
}

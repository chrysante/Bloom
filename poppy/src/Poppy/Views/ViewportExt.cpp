#include "Poppy/Views/ViewportExt.h"

#include <imgui.h>
#include <utl/functional.hpp>
#include <utl/vector.hpp>

#include "Bloom/Graphics/Camera.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/Editor/EntitySelectionManager.h"

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
    if (ImGui::IsKeyDown(ImGuiKey_ModShift)) {
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

static uint32_t toUCol32(float4 color) {
    return ImGui::ColorConvertFloat4ToU32(color);
}

static uint32_t toUCol32(float3 color, float alpha = 1.0f) {
    return toUCol32(float4{ color, alpha });
}

static void drawSpotlightVizWS(EntityHandle entity, float radius, float angle,
                               float3 color,
                               utl::function_view<float3(float3)> WStoSS) {
    auto* DL = ImGui::GetWindowDrawList();
    int segments = 32;
    utl::small_vector<float3, 32> circlePoints1;
    for (int i = 0; i < segments; ++i) {
        float tau = vml::constants<>::pi * 2;
        float angle = i * tau / segments;
        circlePoints1.push_back({ 0, cos(angle), sin(angle) });
    }
    float theta = angle;
    float r1 = radius;
    float r2 = radius + 50;
    float rho1 = r1 * sin(theta);
    float rho2 = r2 * sin(theta);
    float sigma1 = r1 * cos(theta);
    float sigma2 = r2 * cos(theta);
    float4x4 transform = entity.scene().calculateWorldTransformMatrix(entity);
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
                   std::back_inserter(pointsScreenSpace1),
                   [&](float3 x) { return WStoSS(x).xy; });
    utl::small_vector<float2, 32> pointsScreenSpace2;
    std::transform(circlePoints2.begin(), circlePoints2.end(),
                   std::back_inserter(pointsScreenSpace2),
                   [&](float3 x) { return WStoSS(x).xy; });

    for (int i = 0; i < segments; i += 4) {
        DL->AddLine(pointsScreenSpace1[i], pointsScreenSpace2[i],
                    toUCol32(color));
    }
    for (int i = 0; i < segments; ++i) {
        DL->AddLine(pointsScreenSpace1[i],
                    pointsScreenSpace1[(i + 1) % segments], toUCol32(color));
        DL->AddLine(pointsScreenSpace2[i],
                    pointsScreenSpace2[(i + 1) % segments], toUCol32(color));
    }
}

static void drawArrow(float2 from, float2 to, float headSize, float4 color,
                      float thickness) {
    float2 p0 = from;
    float2 line = to - from;
    float angle = std::acos(dot(normalize(line), float2(1, 0)));
    float2x2 arrowRot = { std::cos(angle), -std::sin(angle), std::sin(angle),
                          std::cos(angle) };
    float2 t0 = arrowRot * headSize * float2{ 0, 0 };
    float2 t1 = arrowRot * headSize * float2{ -std::sqrt(3) / 2, -0.5 };
    float2 t2 = arrowRot * headSize * float2{ -std::sqrt(3) / 2, 0.5 };
    auto* DL = ImGui::GetWindowDrawList();
    std::array<float2, 5> const positions = {
        p0, p0 + (norm(line) - headSize) * normalize(line), p0 + line + t0,
        p0 + line + t1, p0 + line + t2
    };
    DL->AddLine(positions[0], positions[1], toUCol32(color), thickness);
    DL->AddTriangle(positions[2], positions[3], positions[4], toUCol32(color),
                    thickness);
}

template <typename LightType>
static void doDisplayLightOverlays(std::span<Scene* const> scenes,
                                   EntitySelectionManager const& selection,
                                   utl::function_view<float3(float3)> WStoSS,
                                   auto draw, EntityHandle& clicked) {
    for (auto* scene: scenes) {
        auto view =
            scene->view<TransformMatrixComponent const, LightType const>();
        view.each([&](EntityID ID, TransformMatrixComponent const& transform,
                      LightType const& light) {
            float3 positionSS = WStoSS(transform.matrix.column(3).xyz);
            if (positionSS.z < 0 || positionSS.z > 1) {
                return;
            }
            EntityHandle entity = scene->getHandle(ID);
            draw(entity, selection.contains(entity), positionSS.xy,
                 Transform::fromMatrix(transform.matrix), light);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                vml::distance(positionSS.xy, (float2)ImGui::GetMousePos()) < 30)
            {
                clicked = entity;
            }
        });
    }
}

namespace {

struct Draw {
    utl::function_view<float3(float3)> WStoSS;

    void operator()(EntityHandle, bool selected, float2 position,
                    Transform const&, PointLightComponent const& light) const {
        float3 color = light.color;
        float size = 20;
        float alphaFactor = selected ? 1.0 : 0.5;
        auto* DL = ImGui::GetWindowDrawList();
        DL->AddCircle(position, size, toUCol32({ 0, 0, 0, alphaFactor * 0.5 }),
                      /* segments = */ 0,
                      /* thickness = */ 4);
        DL->AddCircle(position, size, toUCol32(color, alphaFactor),
                      /* segments = */ 0,
                      /* thickness = */ 2);
    }

    void operator()(EntityHandle entity, bool selected, float2 position,
                    Transform const&, SpotLight const& light) const {
        float3 color = light.color;
        float radius = 25;
        float2 v0 = float2(0, 1) * radius + position;
        float angle1 = vml::constants<>::pi * (0.5 + 2. / 3.);
        float2 v1 =
            float2(std::cos(angle1), std::sin(angle1)) * radius + position;
        float angle2 = vml::constants<>::pi * (0.5 + 4. / 3.);
        float2 v2 =
            float2(std::cos(angle2), std::sin(angle2)) * radius + position;

        auto* DL = ImGui::GetWindowDrawList();
        DL->AddTriangle(v0, v1, v2, toUCol32({ 0, 0, 0, 0.5 }), 4);
        DL->AddTriangle(v0, v1, v2, toUCol32(color), 2);
        if (selected) {
            drawSpotlightVizWS(entity, light.radius,
                               (light.innerCutoff + light.outerCutoff) / 2,
                               (light.color + vml::colors<float3>::white) / 2,
                               WStoSS);
        }
    }

    void operator()(EntityHandle, bool /*selected*/, float2 position,
                    Transform const&,
                    DirectionalLightComponent const& light) const {
        float3 color = light.color;
        float size = 18;
        float2 line = float2(-1, 2) * size;
        auto doDraw = [&](float4 color, float thickness) {
            for (auto offset: { -1, 0, 1 }) {
                float2 const p = position + float2(offset + 0.5, -1) * size;
                drawArrow(p, p + line, size * 0.6, color, thickness);
            }
        };
        doDraw(float4(0, 0, 0, 0.5), 4);
        doDraw(float4(color, 1), 2);
    }

    void operator()(EntityHandle, bool /*selected*/, float2 position,
                    Transform const&, SkyLightComponent const& light) const {
        float3 color = light.color;
        float size = 20;
        float lineSize = size;
        auto doDraw = [&](float4 color, float thickness) {
            std::array<ImVec2, 9> points{};
            for (int i = 0; i < 9; ++i) {
                float angle =
                    vml::constants<>::pi * (0.5 + (i - 4) / 4.0 * 0.25);
                float2 dir = float2(std::cos(-angle), std::sin(-angle));
                float2 from = position + (float2(0, 1) + dir * 2) * size;
                points[i] = from;
                if (i % 2) {
                    continue;
                }
                float2 line = -dir * lineSize;
                drawArrow(from, from + line, 0.3 * lineSize, color, thickness);
            }
            ImGui::GetWindowDrawList()->AddPolyline(points.data(), 9,
                                                    toUCol32(color),
                                                    ImDrawFlags_None, 2);
        };
        doDraw(float4(0, 0, 0, 0.5), 4);
        doDraw(float4(color, 1), 2);
    }
};

} // namespace

bloom::EntityHandle poppy::displayLightOverlays(
    std::span<Scene* const> scenes, EntitySelectionManager const& selection,
    utl::function_view<float3(float3)> WStoSS) {
    auto draw = Draw{ WStoSS };
    EntityHandle clicked;
    doDisplayLightOverlays<PointLightComponent>(scenes, selection, WStoSS, draw,
                                                clicked);
    doDisplayLightOverlays<SpotLightComponent>(scenes, selection, WStoSS, draw,
                                               clicked);
    doDisplayLightOverlays<DirectionalLightComponent>(scenes, selection, WStoSS,
                                                      draw, clicked);
    doDisplayLightOverlays<SkyLightComponent>(scenes, selection, WStoSS, draw,
                                              clicked);
    return clicked;
}

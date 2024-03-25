#include "Poppy/Editor/Views/Viewport/ViewportCameraController.h"

#include <utl/functional.hpp>

#include "Poppy/Core/Debug.h"

using namespace poppy;

std::string poppy::toString(Projection proj) {
    return std::array{ "Perspective", "Orthogonal" }[(size_t)proj];
}

void ViewportCameraController::update(bloom::Timestep time,
                                      bloom::Input const& input) {
    using namespace bloom;

    data.angleLR = utl::mod(data.angleLR - input.mouseOffset().x / 180.,
                            vml::constants<>::pi * 2);
    data.angleUD = std::clamp(data.angleUD + input.mouseOffset().y / 180.f,
                              0.01f, vml::constants<float>::pi - 0.01f);

    float offset = data.speed * time.delta;
    if (input.keyDown(bloom::Key::LeftShift)) {
        offset *= 3;
    }

    if (input.keyDown(Key::A)) {
        data.position -= vml::normalize(vml::cross(front(), up())) * offset;
    }
    if (input.keyDown(Key::D)) {
        data.position += vml::normalize(vml::cross(front(), up())) * offset;
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

void ViewportCameraController::applyProjection(vml::float2 screenSize) {
    if (data.projection == Projection::Perspective) {
        cam.setProjection(vml::to_radians(data.fieldOfView), screenSize,
                          data.nearClip);
    }
    else {
        float const width = 1000; // ??
        float const aspect = screenSize.y / screenSize.x;
        float const height = width * aspect;
        cam.setProjectionOrtho(-width / 2, width / 2, -height / 2, height / 2,
                               0, 5000);
    }
}

vml::float3 ViewportCameraController::front() const {
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

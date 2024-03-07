#include "Poppy/Editor/Views/Viewport/ViewportCameraController.hpp"

#include <utl/functional.hpp>

#include "Poppy/Core/Debug.hpp"

using namespace poppy;

std::string poppy::toString(Projection proj) {
    return std::array{ "Perspective", "Orthogonal" }[(std::size_t)proj];
}

void ViewportCameraController::update(bloom::Timestep time,
                                      bloom::Input const& input) {
    using namespace bloom;

    data.angleLR = utl::mod(data.angleLR - input.mouseOffset().x / 180.,
                            mtl::constants<>::pi * 2);
    data.angleUD = std::clamp(data.angleUD + input.mouseOffset().y / 180.f,
                              0.01f, mtl::constants<float>::pi - 0.01f);

    float offset = data.speed * time.delta;
    if (input.keyDown(bloom::Key::LeftShift)) {
        offset *= 3;
    }

    if (input.keyDown(Key::A)) {
        data.position -= mtl::normalize(mtl::cross(front(), up())) * offset;
    }
    if (input.keyDown(Key::D)) {
        data.position += mtl::normalize(mtl::cross(front(), up())) * offset;
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
    camera.setTransform(data.position, front());
}

void ViewportCameraController::applyProjection(mtl::float2 screenSize) {
    if (data.projection == Projection::perspective) {
        camera.setProjection(mtl::to_radians(data.fieldOfView), screenSize,
                             data.nearClip);
    }
    else {
        float const width = 1000; // ??
        float const aspect = screenSize.y / screenSize.x;
        float const height = width * aspect;
        camera.setProjectionOrtho(-width / 2, width / 2, -height / 2,
                                  height / 2, 0, 5000);
    }
}

mtl::float3 ViewportCameraController::front() const {
    using std::cos;
    using std::sin;
    return { sin(data.angleUD) * cos(data.angleLR),
             sin(data.angleUD) * sin(data.angleLR), cos(data.angleUD) };
}

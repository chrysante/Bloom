#ifndef POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTCAMERACONTROLLER_H
#define POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTCAMERACONTROLLER_H

#include <mtl/mtl.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/Input.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Graphics/Camera.h"

namespace poppy {

enum class Projection { perspective = 0, orthogonal, _count };

std::string toString(Projection);

struct ViewportCameraController {
    ViewportCameraController() { applyTransform(); }
    void update(bloom::Timestep, bloom::Input const&);

    void applyTransform();
    void applyProjection(mtl::float2 screenSize);

    mtl::float3 front() const;
    mtl::float3 up() const { return { 0, 0, 1 }; }

    struct Data {
        float angleLR = mtl::constants<>::pi / 2;
        float angleUD = mtl::constants<>::pi / 2;
        float speed = 500;
        mtl::float3 position = { 0, -5, 1 };
        Projection projection = Projection::perspective;
        float fieldOfView = 60;
        float nearClip = 1;
    } data;
    bloom::Camera camera;
};

} // namespace poppy

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewportCameraController::Data, angleLR,
                           angleUD, speed, position, projection, fieldOfView,
                           nearClip);

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewportCameraController, data);

#endif // POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTCAMERACONTROLLER_H

#include "Bloom/Graphics/Camera.hpp"

using namespace bloom;

void Camera::setProjection(float fieldOfView,
                           mtl::float2 viewportSize,
                           float nearClipPlane) {
    _fov          = fieldOfView;
    _viewportSize = viewportSize;
    _near         = nearClipPlane;
    _projection =
        mtl::infinite_perspective<mtl::right_handed>(fieldOfView,
                                                     viewportSize.x /
                                                         viewportSize.y,
                                                     nearClipPlane);
}

void Camera::setProjectionOrtho(
    float left, float right, float bottom, float top, float near, float far) {
    _viewportSize = { right - left, top - bottom };
    _projection =
        mtl::ortho<mtl::right_handed>(left, right, bottom, top, near, far);
}

void Camera::setTransform(mtl::float3 position,
                          mtl::float3 front,
                          mtl::float3 up) {
    _position = position;
    _front    = front;
    _up       = up;
    _view     = mtl::look_at<mtl::right_handed>(position, position + front, up);
}

mtl::float3 Camera::right() const {
    return mtl::normalize(mtl::cross(_front, _up));
}

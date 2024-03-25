#include "Bloom/Graphics/Camera.h"

using namespace bloom;

void Camera::setProjection(float fieldOfView, vml::float2 viewportSize,
                           float nearClipPlane) {
    _fov = fieldOfView;
    _viewportSize = viewportSize;
    _near = nearClipPlane;
    _projection =
        vml::infinite_perspective<vml::right_handed>(fieldOfView,
                                                     viewportSize.x /
                                                         viewportSize.y,
                                                     nearClipPlane);
}

void Camera::setProjectionOrtho(float left, float right, float bottom,
                                float top, float near, float far) {
    _viewportSize = { right - left, top - bottom };
    _projection =
        vml::ortho<vml::right_handed>(left, right, bottom, top, near, far);
}

void Camera::setTransform(vml::float3 position, vml::float3 front,
                          vml::float3 up) {
    _position = position;
    _front = front;
    _up = up;
    _view = vml::look_at<vml::right_handed>(position, position + front, up);
}

vml::float3 Camera::right() const {
    return vml::normalize(vml::cross(_front, _up));
}

#ifndef BLOOM_GRAPHICS_CAMERA_H
#define BLOOM_GRAPHICS_CAMERA_H

#include <vml/vml.hpp>

#include "Bloom/Core/Base.h"

namespace bloom {

class BLOOM_API Camera {
public:
    void setProjection(float fieldOfView, vml::float2 viewportSize,
                       float nearClipPlane = 0.01);
    void setProjectionOrtho(float left, float right, float bottom, float top,
                            float near, float far);
    void setTransform(vml::float3 position, vml::float3 front,
                      vml::float3 up = { 0, 0, 1 });

    vml::float4x4 const& view() const { return _view; }
    vml::float4x4 const& projection() const { return _projection; }
    vml::float4x4 viewProjection() const { return _projection * _view; }

    vml::float3 position() const { return _position; }
    vml::float3 front() const { return _front; }
    vml::float3 up() const { return _up; }
    vml::float3 right() const;

    vml::float2 viewportSize() const { return _viewportSize; }
    float aspectRatio() const { return _viewportSize.x / _viewportSize.y; }
    /// Only valid with perspective projection matrix
    float fieldOfView() const { return _fov; }
    /// Only valid with perspective projection matrix
    float nearClipPlane() const { return _near; }

private:
    vml::float4x4 _view = 0, _projection = 0;
    // transform
    vml::float3 _position = 0;
    vml::float3 _front = { 0, 1, 0 };
    vml::float3 _up = { 0, 0, 1 };
    // projection
    vml::float2 _viewportSize = 0;
    float _fov = 1.0471; // 60 degrees
    float _near;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_CAMERA_H

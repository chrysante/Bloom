#pragma once

#include <mtl/mtl.hpp>

#include "Bloom/Core/Base.hpp"

namespace bloom {

class BLOOM_API Camera {
public:
    void setProjection(float fieldOfView,
                       mtl::float2 viewportSize,
                       float nearClipPlane = 0.01);
    void setProjectionOrtho(float left,
                            float right,
                            float bottom,
                            float top,
                            float near,
                            float far);
    void setTransform(mtl::float3 position,
                      mtl::float3 front,
                      mtl::float3 up = { 0, 0, 1 });

    mtl::float4x4 const& view() const { return _view; }
    mtl::float4x4 const& projection() const { return _projection; }
    mtl::float4x4 viewProjection() const { return _projection * _view; }

    mtl::float3 position() const { return _position; }
    mtl::float3 front() const { return _front; }
    mtl::float3 up() const { return _up; }
    mtl::float3 right() const;

    mtl::float2 viewportSize() const { return _viewportSize; }
    float aspectRatio() const { return _viewportSize.x / _viewportSize.y; }
    /// Only valid with perspective projection matrix
    float fieldOfView() const { return _fov; }
    /// Only valid with perspective projection matrix
    float nearClipPlane() const { return _near; }

private:
    mtl::float4x4 _view = 0, _projection = 0;
    // transform
    mtl::float3 _position = 0;
    mtl::float3 _front    = { 0, 1, 0 };
    mtl::float3 _up       = { 0, 0, 1 };
    // projection
    mtl::float2 _viewportSize = 0;
    float _fov                = 1.0471; // 60 degrees
    float _near;
};

} // namespace bloom

#include "Camera.hpp"

namespace bloom {
	
	void Camera::setProjection(float fieldOfView, mtl::float2 viewportSize, float nearClipPlane) {
		projection = mtl::infinite_perspective<mtl::right_handed>(fieldOfView,
																  viewportSize.x / viewportSize.y,
																  nearClipPlane);
	}
	
	void Camera::setProjectionOrtho(float left, float right,
									float bottom, float top,
									float near, float far)
	{
		projection = mtl::ortho<mtl::right_handed>(left, right, bottom, top, near, far);
	}
	
	void Camera::setTransform(mtl::float3 position, mtl::float3 front, mtl::float3 up) {
		view = mtl::look_at<mtl::right_handed>(position, position + front, up);
	}
	
	mtl::float4x4 Camera::getMatrix() const {
		return projection * view;
	}
	
	mtl::float3 Camera::getPosition() const {
		auto const inverseView = mtl::inverse(view);
		return inverseView.column(3).xyz;
	}
	
}

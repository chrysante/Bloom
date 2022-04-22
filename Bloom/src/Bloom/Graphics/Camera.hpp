#pragma once

#include "Bloom/Core/Base.hpp"

#include <mtl/mtl.hpp>

namespace bloom {
	
	class BLOOM_API Camera {
	public:
		void setProjection(float fieldOfView, mtl::float2 viewportSize, float nearClipPlane = 0.01);
		void setProjectionOrtho(float left, float right, float bottom, float top, float near, float far);
		void setTransform(mtl::float3 position, mtl::float3 front, mtl::float3 up = { 0, 0, 1 });
		mtl::float4x4 getMatrix() const;
		
		mtl::float4x4 const& getView() const { return view; };
		mtl::float4x4 const& getProjection() const { return projection; };
		
		mtl::float3 getPosition() const;
		
	private:
		mtl::float4x4 view = 0, projection = 0;
	};
	
}

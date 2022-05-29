#pragma once

#include "Bloom/Core/Time.hpp"
#include "Bloom/Core/Serialize.hpp"
#include "Bloom/Application/Input.hpp"
#include "Bloom/Graphics/Camera.hpp"

#include <mtl/mtl.hpp>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/helpers.hpp>

namespace poppy {
	
	enum class Projection {
		perspective = 0, orthogonal, _count
	};
	std::string toString(Projection);
	
	struct ViewportCameraController {
		ViewportCameraController() {
			applyTransform();
		}
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
	
}

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewportCameraController::Data,
						   angleLR,
						   angleUD,
						   speed,
						   position,
						   projection,
						   fieldOfView,
						   nearClip);

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewportCameraController,
						   data);

#pragma once

#include "Bloom/Core/Time.hpp"
#include "Bloom/Application/Input.hpp"
#include "Bloom/Graphics/Camera.hpp"

#include <mtl/mtl.hpp>
#include <yaml-cpp/yaml.h>
#include <yaml-cpp/helpers.hpp>

namespace poppy {
	
	struct ViewportCameraActor {
		ViewportCameraActor() {
			camera.setTransform(position, front());
		}
		void update(bloom::TimeStep, bloom::Input const&);
		
		void applyTransform();
		
		bloom::Camera camera;
		float angleLR = mtl::constants<>::pi / 2;
		float angleUD = mtl::constants<>::pi / 2;
		float speed = 500;
		mtl::float3 position = { 0, -5, 1 };
		mtl::float3 front() const;
		mtl::float3 up() const { return { 0, 0, 1 }; }
	};
	
}


template<> struct YAML::convert<poppy::ViewportCameraActor> {
	static Node encode(poppy::ViewportCameraActor const& c) {
		Node node;
		node["Angle Left-Right"] = c.angleLR;
		node["Angle Up-Down"] = c.angleUD;
		node["Speed"] = c.speed;
		node["Position"] = c.position;
		return node;
	}
	
	static bool decode(Node const& node, poppy::ViewportCameraActor& c) {
		try {
			c.angleLR  = node["Angle Left-Right"].as<float>();
			c.angleUD  = node["Angle Up-Down"].as<float>();
			c.speed    = node["Speed"].as<float>();
			c.position = node["Position"].as<mtl::float3>();
			return true;
		}
		catch (InvalidNode const&) {
			return false;
		}
	}
};

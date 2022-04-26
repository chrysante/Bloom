#include "ViewportCameraActor.hpp"

#include "Poppy/Debug.hpp"

#include <utl/functional.hpp>

namespace poppy {
	
	void ViewportCameraActor::update(bloom::TimeStep time, bloom::Input const& input) {
		using namespace bloom;
		
		angleLR  = utl::mod(angleLR - input.mouseOffset().x / 180., mtl::constants<>::pi * 2);
		angleUD  = std::clamp(angleUD + input.mouseOffset().y / 180.f, 0.01f, mtl::constants<float>::pi - 0.01f);
		
		float offset = speed * time.delta;
		if (input.keyDown(bloom::Key::leftShift)) {
			offset *= 3;
		}
		
		if (input.keyDown(Key::A)) {
			position -= mtl::normalize(mtl::cross(front(), up())) * offset;
		}
		if (input.keyDown(Key::D)) {
			position += mtl::normalize(mtl::cross(front(), up())) * offset;
		}
		if (input.keyDown(Key::W)) {
			position += front() * offset;
		}
		if (input.keyDown(Key::S)) {
			position -= front() * offset;
		}
		if (input.keyDown(Key::Q)) {
			position -= up() * offset;
		}
		if (input.keyDown(Key::E)) {
			position += up() * offset;
		}
		
		applyTransform();
	}
	
	void ViewportCameraActor::applyTransform() {
		camera.setTransform(position, front());
	}
	
	mtl::float3 ViewportCameraActor::front() const {
		using std::sin;
		using std::cos;
		return {
			sin(angleUD) * cos(angleLR),
			sin(angleUD) * sin(angleLR),
			cos(angleUD)
		};
	}
	
}

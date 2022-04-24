#include <Catch2/Catch2.hpp>

#include <iostream>
#include <utl/vector.hpp>

#include <yaml-cpp/yaml.h>

#include <mtl/mtl.hpp>

struct Camera {
	float fieldOfView;
	float position;
};


TEST_CASE() {
	
	using namespace mtl;
	
	int segments = 30;
	
	utl::vector<float2> positions;
	
	for (int i = 0; i < segments + 1; ++i) {
		float const tau = mtl::constants<>::pi * 2;
		float const angle = i * tau / segments;
		positions.push_back(float2{ std::cos(angle), std::sin(angle) });
	}
	
	
	
	
	
	
	
	
	
	for (auto p: positions) {
		std::cout << p << std::endl;
	}
	
}

#include "Components.hpp"

namespace bloom {
	
	mtl::float4x4 TransformComponent::calculate() const {
		return mtl::make_transform(position, orientation, scale);
	}
	
}

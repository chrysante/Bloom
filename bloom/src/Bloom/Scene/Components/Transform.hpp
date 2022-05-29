#pragma once

#include "ComponentBase.hpp"

#include <mtl/mtl.hpp>

namespace bloom {
	
	struct BLOOM_API Transform {
		BLOOM_REGISTER_COMPONENT("Transform");
		
		mtl::float3 position = 0;
		mtl::quaternion_float orientation = 1;
		mtl::float3 scale = 1;
		
		static Transform fromMatrix(mtl::float4x4 const& m) {
			auto const [t, r, s] = mtl::decompose_transform(m);
			return { t, r, s };
		}
		
		[[ nodiscard ]] mtl::float4x4 calculate() const {
			return mtl::make_transform(position, orientation, scale);
		}
	};
	
	struct BLOOM_API TransformMatrixComponent {
		BLOOM_REGISTER_COMPONENT("Transform Matrix");
		mtl::float4x4 matrix;
	};
	
}

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(bloom::Transform,
						   position,
						   orientation,
						   scale);

#endif

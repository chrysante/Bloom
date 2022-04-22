#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"

#include "RenderPrimitives.hpp"

namespace bloom {
	
	class BLOOM_API StaticMesh {
		struct PrivateTag{};
		StaticMesh() = default;
	public:
		StaticMesh(PrivateTag): StaticMesh() {}
		
	public:
		
		static Reference<StaticMesh> create();
		
//	private:
		BufferHandle vertexBuffer, indexBuffer;
	};
	
}

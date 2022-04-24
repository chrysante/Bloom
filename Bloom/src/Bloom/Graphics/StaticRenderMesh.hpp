#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"

#include "RenderPrimitives.hpp"
#include "Bloom/Assets/Asset.hpp"

namespace bloom {
	
	class BLOOM_API StaticRenderMesh {
	public: // for now
		struct PrivateTag{};
		StaticRenderMesh() = default;
	public:
		StaticRenderMesh(PrivateTag): StaticRenderMesh() {}
		
	public:
		
		static Reference<StaticRenderMesh> create();
		
		AssetHandle assetHandle() const { return _handle; }
		
//	private:
		AssetHandle _handle;
		BufferHandle vertexBuffer, indexBuffer;
	};
	
}

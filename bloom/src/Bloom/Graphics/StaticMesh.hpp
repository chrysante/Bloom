#pragma once

#include "Vertex.hpp"

#include "Bloom/Core/Core.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"
#include "Bloom/Asset/Asset.hpp"

#include <utl/vector.hpp>

namespace bloom {
	
	class StaticMeshData;
	class StaticMeshRenderer;
	
	class BLOOM_API StaticMesh: public Asset {
		friend class AssetManager;
		
	public:
		using Asset::Asset;
		
		Reference<StaticMeshData> getData() { return mData; }
		Reference<StaticMeshRenderer> getRenderer() { return mRenderer; }
		
	private:
		Reference<StaticMeshData> mData;
		Reference<StaticMeshRenderer> mRenderer;
	};
	
	struct BLOOM_API StaticMeshData {
		utl::vector<Vertex3D> vertices;
		utl::vector<std::uint32_t> indices;
	};
	
	class BLOOM_API StaticMeshRenderer {
		friend class AssetManager;
		
	public:
		BufferView vertexBuffer() const { return mVertexBuffer; }
		BufferView indexBuffer() const { return mIndexBuffer; }
		
	private:
		BufferHandle mVertexBuffer, mIndexBuffer;
	};
	
}

#pragma once

#include "Asset.hpp"

#include "Bloom/Core/Reference.hpp"
#include "Bloom/Graphics/Vertex.hpp"
#include "Bloom/Graphics/Material.hpp"
#include "Bloom/Scene/Scene.hpp"

#include <utl/vector.hpp>

#include "ScriptAsset.hpp"

namespace bloom {
	
	/// MARK: Static Mesh
	class StaticRenderMesh;
	
	struct StaticMeshData {
		utl::vector<Vertex3D> vertices;
		utl::vector<std::uint32_t> indices;
	};
	
	class BLOOM_API StaticMeshAsset: public Asset {
		friend class AssetManager;
		
	public:
		using Asset::Asset;
		
		Reference<StaticMeshData> getMeshData() { return _meshData; }
		Reference<StaticRenderMesh> getRenderMesh() { return _renderMesh; }
		
	private:
		Reference<StaticMeshData> _meshData;
		Reference<StaticRenderMesh> _renderMesh;
	};
	
	/// MARK: Material
	class BLOOM_API MaterialAsset: public Asset {
	public:
		using Asset::Asset;
		
		Material material;
	};
	
	
	/// MARK: Scene
	class BLOOM_API SceneAsset: public Asset {
	public:
		using Asset::Asset;
		
		Scene scene;
	};
	
}

#pragma once

#include "ComponentBase.hpp"

#include "Bloom/Graphics/Material.hpp"
#include "Bloom/Graphics/StaticRenderMesh.hpp"

namespace bloom {
	
	struct BLOOM_API MeshRendererComponent {
		BLOOM_REGISTER_COMPONENT("Mesh Renderer");
		
		Reference<MaterialAsset> material;
		Reference<StaticMeshAsset> mesh;
	};
	
}

#ifdef BLOOM_CPP

#include "Bloom/Core/Serialize.hpp"

namespace bloom {
	class AssetManager;
	
	YAML::Node encode(MeshRendererComponent const&);
	MeshRendererComponent decodeMeshRendererComponent(YAML::Node const&, AssetManager*);
}

#endif

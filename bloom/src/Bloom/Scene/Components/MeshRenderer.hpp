#pragma once

#include "ComponentBase.hpp"

#include "Bloom/Graphics/Material/MaterialInstance.hpp"
#include "Bloom/Graphics/StaticMesh.hpp"

namespace bloom {
	
	class AssetManager;
	
	struct BLOOM_API MeshRendererComponent {
		BLOOM_REGISTER_COMPONENT("Mesh Renderer");
		
		Reference<MaterialInstance> materialInstance;
		Reference<StaticMesh> mesh;
		
		YAML::Node serialize() const;
		void deserialize(YAML::Node const&, AssetManager&);
	};
	
}

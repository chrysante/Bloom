#include "MeshRenderer.hpp"

#include "Bloom/Asset/AssetManager.hpp"

namespace bloom {
	
	YAML::Node MeshRendererComponent::serialize() const {
		YAML::Node node;
		node["MaterialInstance"] = materialInstance ? materialInstance->handle() : AssetHandle{};
		node["Mesh"] = mesh ? mesh->handle() : AssetHandle{};
		return node;
	}
	
	void MeshRendererComponent::deserialize(YAML::Node const& node, AssetManager& assetManager) {
		auto const matHandle = node["MaterialInstance"].as<AssetHandle>();
		auto const meshHandle = node["Mesh"].as<AssetHandle>();
		
		materialInstance = as<MaterialInstance>(assetManager.get(matHandle));
		mesh = as<StaticMesh>(assetManager.get(meshHandle));
	
		assetManager.makeAvailable(matHandle, AssetRepresentation::GPU);
		assetManager.makeAvailable(meshHandle, AssetRepresentation::GPU);
	}
	
}

#include "MeshRenderer.hpp"

#include "Bloom/Asset/AssetManager.hpp"

namespace bloom {
	
	YAML::Node MeshRendererComponent::serialize() const {
		YAML::Node node;
		node["Material"] = material ? material->handle() : AssetHandle{};
		node["Mesh"] = mesh ? mesh->handle() : AssetHandle{};
		return node;
	}
	
	void MeshRendererComponent::deserialize(YAML::Node const& node, AssetManager& assetManager) {
		auto const matHandle = node["Material"].as<AssetHandle>();
		auto const meshHandle = node["Mesh"].as<AssetHandle>();
		
		material = as<Material>(assetManager.get(matHandle)),
		mesh = as<StaticMesh>(assetManager.get(meshHandle)),
	
		assetManager.makeAvailable(matHandle, AssetRepresentation::GPU);
		assetManager.makeAvailable(meshHandle, AssetRepresentation::GPU);
	}
	
}

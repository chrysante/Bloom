#include "MeshRenderer.hpp"

#include "Bloom/Assets/AssetManager.hpp"
#include "Bloom/Assets/ConcreteAssets.hpp"

namespace bloom {
	
	YAML::Node encode(MeshRendererComponent const& m) {
		YAML::Node node;
		node["Material"] = m.material ? m.material->handle() : AssetHandle{};
		node["Mesh"] = m.mesh ? m.mesh->handle() : AssetHandle{};
		return node;
	}
	
	MeshRendererComponent decodeMeshRendererComponent(YAML::Node const& node, AssetManager* manager) {
		auto const matHandle = node["Material"].as<AssetHandle>();
		auto const meshHandle = node["Mesh"].as<AssetHandle>();
		auto const result = MeshRendererComponent{
			.material = as<MaterialAsset>(manager->get(matHandle)),
			.mesh = as<StaticMeshAsset>(manager->get(meshHandle)),
		};
		manager->makeAvailable(matHandle, AssetRepresentation::GPU);
		manager->makeAvailable(meshHandle, AssetRepresentation::GPU);
		return result;
	}
	
}

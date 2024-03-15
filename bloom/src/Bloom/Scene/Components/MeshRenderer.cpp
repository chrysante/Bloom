#include "Bloom/Scene/Components/MeshRenderer.h"

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/Serialization.h"

using namespace bloom;

YAML::Node MeshRendererComponent::serialize() const {
    YAML::Node node;
    node["MaterialInstance"] = materialInstance ? materialInstance->handle() :
                                                  AssetHandle{};
    node["Mesh"] = mesh ? mesh->handle() : AssetHandle{};
    return node;
}

void MeshRendererComponent::deserialize(YAML::Node const& node,
                                        AssetManager& assetManager) {
    auto const matHandle = node["MaterialInstance"].as<AssetHandle>();
    auto const meshHandle = node["Mesh"].as<AssetHandle>();

    materialInstance = as<MaterialInstance>(assetManager.get(matHandle));
    mesh = as<StaticMesh>(assetManager.get(meshHandle));

    assetManager.makeAvailable(matHandle, AssetRepresentation::GPU);
    assetManager.makeAvailable(meshHandle, AssetRepresentation::GPU);
}

#include "Bloom/Scene/Components/MeshRenderer.h"

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/Serialization.h"

using namespace bloom;
using enum AssetRepresentation;

MeshRendererComponent::MeshRendererComponent(WeakType const& weak):
    materialInstance(
        getAsset<MaterialInstance>(weak.materialInstance, GPU | CPU)),
    mesh(getAsset<StaticMesh>(weak.mesh, GPU | CPU)) {}

static AssetHandle getHandle(Asset const* asset) {
    return asset ? asset->handle() : AssetHandle();
}

MeshRendererComponent::operator WeakType() const {
    return { getHandle(materialInstance.get()), getHandle(mesh.get()) };
}

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

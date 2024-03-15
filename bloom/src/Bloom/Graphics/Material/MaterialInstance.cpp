#include "Bloom/Graphics/Material/MaterialInstance.h"

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/Serialization.h"

namespace bloom {

void MaterialInstance::setMaterial(Reference<Material> material) {
    mMaterial = std::move(material);
}

void MaterialInstance::setParamaters(MaterialParameters const& params) {
    mParameters = params;
    mDirty = true;
}

YAML::Node MaterialInstance::serialize() const {
    YAML::Node root;
    root["Material"] = mMaterial ? mMaterial->handle() : AssetHandle{};
    root["Parameters"] = mParameters;
    return root;
}

void MaterialInstance::deserialize(YAML::Node const& root,
                                   AssetManager& assetManager) {
    mParameters = root["Parameters"].as<MaterialParameters>();
    auto const matHandle = root["Material"].as<AssetHandle>();
    mMaterial = as<Material>(assetManager.get(matHandle));
    assetManager.makeAvailable(matHandle, AssetRepresentation::GPU);
}

} // namespace bloom

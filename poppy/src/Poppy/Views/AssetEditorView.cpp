#include "Poppy/Views/AssetEditorView.h"

#include "Bloom/Asset/Serialization.h"
#include "Bloom/Core/Yaml.h"

using namespace poppy;
using namespace bloom;

YAML::Node AssetEditorViewBase::serialize() const {
    YAML::Node node;
    auto* asset = _getAsset();
    node["Asset"] = asset ? asset->handle() : AssetHandle();
    return node;
}

void AssetEditorViewBase::deserialize(YAML::Node node) {
    auto handle = node["Asset"].as<AssetHandle>(AssetHandle());
    if (!handle) {
        return;
    }
    auto asset = getAsset(handle, AssetRepresentation::CPU);
    if (asset) {
        _setAsset(asset);
        _openAsset(*asset);
    }
}

#ifndef POPPY_VIEWS_ASSETDRAGDROP_H
#define POPPY_VIEWS_ASSETDRAGDROP_H

#include <optional>
#include <span>

#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Reference.h"

namespace poppy {

class Editor;

inline constexpr auto AssetDragDropType = "ASSET_DRAG_DROP";

///
std::optional<bloom::AssetHandle> acceptAssetDragDrop(
    std::span<bloom::AssetType const> types);

/// \overload
inline std::optional<bloom::AssetHandle> acceptAssetDragDrop(
    std::initializer_list<bloom::AssetType> types) {
    return acceptAssetDragDrop(std::span(types));
}

/// \overload
inline std::optional<bloom::AssetHandle> acceptAssetDragDrop(
    bloom::AssetType type) {
    return acceptAssetDragDrop({ type });
}

bool assetButtonImpl(Editor& editor, bloom::Reference<bloom::Asset>& assetRef,
                     bloom::AssetType type, vml::float2 size);

/// Displays a button that can be used to select assets of type \p AssetType
/// The selected asset will be assigned to \p asset
/// \Returns `true` if a new asset has been selected
template <std::derived_from<bloom::Asset> AssetType>
bool assetButton(Editor& editor, bloom::Reference<AssetType>& asset,
                 vml::float2 size = {}) {
    bloom::Reference<bloom::Asset> tmp = asset;
    if (assetButtonImpl(editor, tmp, utl::dc::TypeToID<AssetType>, size)) {
        asset = as<AssetType>(tmp);
        return true;
    }
    return false;
}

/// Opens the asset \p handle in a view
void openAsset(Editor& editor, bloom::AssetHandle handle);

} // namespace poppy

#endif // POPPY_VIEWS_ASSETDRAGDROP_H

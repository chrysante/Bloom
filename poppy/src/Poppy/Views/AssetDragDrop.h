#ifndef POPPY_VIEWS_ASSETDRAGDROP_H
#define POPPY_VIEWS_ASSETDRAGDROP_H

#include <optional>
#include <span>

#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Reference.h"

namespace poppy {

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

enum class AssetButtonStyle { Compact, Thumbnail };

struct AssetButtonDescriptor {
    vml::float2 size = {};
    bloom::AssetType type = bloom::AssetType::Asset;
    AssetButtonStyle style = AssetButtonStyle::Compact;
    bloom::AssetRepresentation repr = bloom::AssetRepresentation::None;
};

bool assetButtonImpl(bloom::Reference<bloom::Asset>& asset,
                     AssetButtonDescriptor desc);

/// Displays a button that can be used to select assets of type \p AssetType
/// The selected asset will be assigned to \p asset
/// \Returns `true` if a new asset has been selected
template <std::derived_from<bloom::Asset> AssetType>
bool assetButton(bloom::Reference<AssetType>& asset,
                 AssetButtonDescriptor desc = {}) {
    desc.type = utl::dc::TypeToID<AssetType>;
    bloom::Reference<bloom::Asset> tmp = asset;
    if (assetButtonImpl(tmp, desc)) {
        asset = as<AssetType>(tmp);
        return true;
    }
    return false;
}

/// Opens the asset \p handle in a view
void openAsset(bloom::AssetHandle handle);

} // namespace poppy

#endif // POPPY_VIEWS_ASSETDRAGDROP_H

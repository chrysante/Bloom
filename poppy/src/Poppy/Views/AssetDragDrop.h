#ifndef POPPY_VIEWS_ASSETDRAGDROP_H
#define POPPY_VIEWS_ASSETDRAGDROP_H

#include <any>
#include <optional>
#include <span>

#include <vml/vml.hpp>

#include "Bloom/Asset/AssetHandle.h"
#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Reference.h"

namespace poppy {

inline constexpr auto AssetDragDropType = "ASSET_DRAG_DROP";

///
std::string selectAssetIcon(bloom::AssetType type);

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
    bloom::AssetRepresentation repr = bloom::AssetRepresentation::None;
    AssetButtonStyle style = AssetButtonStyle::Compact;
};

bool assetButtonImpl(char const* strID, bloom::Reference<bloom::Asset>& asset,
                     bloom::AssetType type, AssetButtonDescriptor const& desc);

/// Displays a button that can be used to select assets of type \p AssetType
/// The selected asset will be assigned to \p asset
/// \Returns `true` if a new asset has been selected
template <std::derived_from<bloom::Asset> AssetType>
bool assetButton(char const* strID, bloom::Reference<AssetType>& asset,
                 AssetButtonDescriptor const& desc = {}) {
    bloom::Reference<bloom::Asset> tmp = asset;
    if (assetButtonImpl(strID, tmp, utl::dc::TypeToID<AssetType>, desc)) {
        asset = as<AssetType>(tmp);
        return true;
    }
    return false;
}

/// Opens the asset \p handle in a view
void openAsset(bloom::AssetHandle handle);

template <std::derived_from<bloom::Asset> AssetType>
bool receiveAsset(
    bloom::Reference<AssetType>& existing, std::any const& incoming,
    bloom::AssetRepresentation repr = bloom::AssetRepresentation::None) {
    auto* inAsset = std::any_cast<bloom::Reference<AssetType>>(&incoming);
    if (!inAsset || inAsset->get() == existing.get()) {
        return false;
    }
    existing = *inAsset;
    return true;
}

} // namespace poppy

#endif // POPPY_VIEWS_ASSETDRAGDROP_H

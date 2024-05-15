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
std::optional<bloom::AssetHandle> acceptAssetDragDrop(
    std::initializer_list<bloom::AssetType> types);

bool assetButtonImpl(bloom::AssetManager& AM,
                     bloom::Reference<bloom::Asset>& assetRef,
                     bloom::AssetType type, vml::float2 size);

template <std::derived_from<bloom::Asset> AssetType>
bool assetButton(bloom::AssetManager& AM, bloom::Reference<AssetType>& asset,
                 vml::float2 size = {}) {
    bloom::Reference<bloom::Asset> tmp = asset;
    if (assetButtonImpl(AM, tmp, utl::dc::TypeToID<AssetType>, size)) {
        asset = as<AssetType>(tmp);
        return true;
    }
    return false;
}

} // namespace poppy

#endif // POPPY_VIEWS_ASSETDRAGDROP_H

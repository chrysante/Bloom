#ifndef POPPY_VIEWS_ASSETDRAGDROP_H
#define POPPY_VIEWS_ASSETDRAGDROP_H

#include <optional>
#include <span>

#include "Bloom/Asset/AssetHandle.h"

namespace poppy {

inline constexpr auto AssetDragDropType = "ASSET_DRAG_DROP";

///
std::optional<bloom::AssetHandle> acceptAssetDragDrop(
    std::span<bloom::AssetType const> types);

/// \overload
std::optional<bloom::AssetHandle> acceptAssetDragDrop(
    std::initializer_list<bloom::AssetType> types);

} // namespace poppy

#endif // POPPY_VIEWS_ASSETDRAGDROP_H

#include "Poppy/Views/AssetDragDrop.h"

#include <imgui.h>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

using namespace poppy;
using namespace bloom;

std::string poppy::toDragDropType(AssetType type) {
    return utl::strcat("DD-Asset-", type);
}

std::optional<AssetHandle> poppy::acceptAssetDragDrop(
    std::span<AssetType const> types) {
    if (!ImGui::BeginDragDropTarget()) {
        return std::nullopt;
    }
    utl::scope_guard endTarget = [] { ImGui::EndDragDropTarget(); };
    for (auto type: types) {
        auto* payload =
            ImGui::AcceptDragDropPayload(toDragDropType(type).data());
        if (!payload) {
            continue;
        }
        if (payload->IsDelivery()) {
            AssetHandle receivedAsset;
            std::memcpy(&receivedAsset, payload->Data, sizeof receivedAsset);
            return receivedAsset;
        }
    }
    return std::nullopt;
}

std::optional<AssetHandle> poppy::acceptAssetDragDrop(
    std::initializer_list<AssetType> types) {
    return acceptAssetDragDrop(std::span(types));
}

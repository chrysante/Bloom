#include "Poppy/Views/AssetDragDrop.h"

#include <imgui.h>
#include <range/v3/algorithm.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Poppy/Views/DirectoryView.h"

using namespace poppy;
using namespace bloom;

std::optional<AssetHandle> poppy::acceptAssetDragDrop(
    std::span<AssetType const> types) {
    if (!ImGui::BeginDragDropTarget()) {
        return std::nullopt;
    }
    utl::scope_guard endTarget = [] { ImGui::EndDragDropTarget(); };
    if (auto* payload = ImGui::GetDragDropPayload();
        !payload || payload->DataType != std::string_view(AssetDragDropType))
    {
        return std::nullopt;
    }
    auto userData = peekFileDragDropUserData();
    if (userData.size() != sizeof(AssetHandle)) {
        return std::nullopt;
    }
    AssetHandle assetHandle;
    std::memcpy(&assetHandle, userData.data(), userData.size());
    if (!ranges::contains(types, assetHandle.type())) {
        return std::nullopt;
    }
    auto* payload = ImGui::AcceptDragDropPayload(AssetDragDropType);
    if (payload && payload->IsDelivery()) {
        return assetHandle;
    }
    return std::nullopt;
}

std::optional<AssetHandle> poppy::acceptAssetDragDrop(
    std::initializer_list<AssetType> types) {
    return acceptAssetDragDrop(std::span(types));
}

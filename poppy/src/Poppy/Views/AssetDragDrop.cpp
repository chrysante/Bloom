#include "Poppy/Views/AssetDragDrop.h"

#include <string>

#include <imgui.h>
#include <range/v3/algorithm.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/AssetManager.h"
#include "Poppy/Views/DirectoryView.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

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

bool poppy::assetButtonImpl(AssetManager& AM, Reference<Asset>& assetRef,
                            AssetType type, float2 size) {
    std::string label = assetRef ? assetRef->name() : "No asset assigned";
    std::string popupName = utl::strcat("AssetBrowserPopup_", type);
    if (ImGui::Button(label.c_str(), size)) {
        ImGui::OpenPopup(popupName.c_str());
    }
    AssetHandle handle;
    if (ImGui::BeginPopup(popupName.c_str())) {
        auto assets = AM.getAssetsOfType(type);
        for (auto asset: assets) {
            bool selected = assetRef && asset == assetRef->handle();
            if (ImGui::Selectable(AM.getName(asset).c_str(), selected)) {
                handle = asset;
            }
        }
        ImGui::EndPopup();
    }
    if (auto payload = acceptAssetDragDrop({ type })) {
        handle = *payload;
    }
    if (handle) {
        assetRef =
            AM.get(handle, AssetRepresentation::CPU | AssetRepresentation::GPU);
    }
    return assetRef != nullptr;
}

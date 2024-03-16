#include "Poppy/Editor/Views/DirectoryView.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/filesystem_ext.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Views/AssetBrowser.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

static std::string toDragDropType(AssetType type) {
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

void DirectoryView::display() {
    ImGui::BeginChild("Asset Browser DirView");
    {
        contentSize = ImGui::GetWindowSize();
        cursor = 0;
        itemIndex = 0;
        for (auto&& folderName: foldersInCurrentDir) {
            if (displayItem(folderName)) {
            }
        }
        for (auto&& asset: assetsInCurrentDir) {
            if (displayItem(assetManager->getName(asset), asset)) {
                browser->openAsset(asset);
            }
        }
        /// Add spacing at the end
        if (cursor.x > 0) {
            bool const forceLineBreak = true;
            advanceCursor(forceLineBreak);
        }
        ImGui::SetCursorPos(cursor);
        ImGui::PushStyleColor(ImGuiCol_Separator, 0);
        ImGui::Separator();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}

static std::string selectIcon(std::optional<AssetType> type) {
    if (!type) {
        return "doc";
    }
    switch (*type) {
    case AssetType::StaticMesh:
        return "cube";
    case AssetType::SkeletalMesh:
        return "cube";
    case AssetType::Material:
        return "delicious";
    case AssetType::MaterialInstance:
        return "delicious";
    case AssetType::Scene:
        return "cubes";
    case AssetType::ScriptSource:
        return "file-code";
    default:
        return "doc";
    }
}

bool DirectoryView::displayItem(std::string_view label,
                                std::optional<AssetHandle> asset) {
    mtl::float2 labelSize = ImGui::CalcTextSize(label.data());
    auto uniqueID = generateUniqueID(label, itemIndex, true);
    auto popupID = generateUniqueID(uniqueID.data(), itemIndex);
    mtl::float2 localCursor = cursor;
    // Button
    localCursor += params.itemSpacing;
    params.itemSize = 96;
    auto iconSize = IconSize::_32;
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x20FFffFF);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x20FFffFF);
    ImGuiButtonFlags flags = 0;
#if 0 // Does not work well with drag and drop
    flags |= ImGuiButtonFlags_PressedOnDoubleClick;
#endif
    ImGui::SetCursorPos(localCursor);
    bool result = ImGui::ButtonEx(uniqueID.data(), params.itemSize, flags);
    ImGui::PopStyleColor(3);
    // Popup
    ImGui::OpenPopupOnItemClick(popupID.data());
    if (ImGui::BeginPopup(popupID.data())) {
        if (ImGui::Selectable("Rename")) {
            renaming = itemIndex;
            setRenameFocus = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    // Drag-drop
    if (asset && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload(toDragDropType(asset->type()).data(),
                                  &asset.value(), sizeof *asset);
        ImGui::PushFont((ImFont*)icons.font(iconSize));
        auto iconText = icons.unicodeStr(
            selectIcon(asset ? std::optional(asset->type()) : std::nullopt));
        ImGui::Text("%s", iconText.data());
        ImGui::PopFont();
        ImGui::EndDragDropSource();
    }
    // Icon
    {
        ImGui::PushFont((ImFont*)icons.font(iconSize));
        auto const iconText = icons.unicodeStr(std::string(
            asset ? selectIcon(asset->type()) : "folder-open-empty"));
        auto iconCursor = localCursor;
        iconCursor.y += (params.itemSize.y - params.labelHeight) / 2;
        iconCursor.x += params.itemSize.x / 2;
        mtl::float2 const iconSize = ImGui::CalcTextSize(iconText.data());
        iconCursor -= iconSize / 2;
        //			iconCursor -= framePadding / 2;
        ImGui::SetCursorPos(iconCursor);
        ImGui::Text("%s", iconText.data());
        ImGui::PopFont();
    }
    // Label
    if (itemIndex == renaming) {
        if (setRenameFocus) {
            std::strncpy(renameBuffer.data(), label.data(),
                         renameBuffer.size() - 1);
        }
        mtl::float2 const labelSize = ImGui::CalcTextSize(renameBuffer.data());
        mtl::float2 const framePadding =
            mtl::float2(GImGui->Style.FramePadding);
        localCursor.y += params.itemSize.y - params.labelHeight / 2;
        localCursor.x += params.itemSize.x / 2;
        localCursor -= labelSize / 2;
        localCursor -= framePadding / 2;
        ImGui::SetCursorPos(localCursor);
        std::strncpy(renameBuffer.data(), label.data(),
                     renameBuffer.size() - 1);
        if (setRenameFocus) {
            ImGui::SetKeyboardFocusHere();
            setRenameFocus = false;
        }
        ImGui::SetNextItemWidth(labelSize.x + 2 * framePadding.x);
        if (ImGui::InputText("##rename-field", renameBuffer.data(),
                             renameBuffer.size(),
                             ImGuiInputTextFlags_EnterReturnsTrue))
        {
            renaming = -1;
            renameBuffer = {};
        }
        Logger::Trace("ItemID: ", ImGui::GetItemID(),
                      ", FocusID: ", ImGui::GetFocusID());
    }
    else {
        localCursor.y += params.itemSize.y - params.labelHeight / 2;
        localCursor.x += params.itemSize.x / 2;
        localCursor -= labelSize / 2;
        ImGui::SetCursorPos(localCursor);
        ImGui::Text("%s", label.data());
    }
    advanceCursor();
    return result;
}

void DirectoryView::advanceCursor(bool forceNextLine) {
    ++itemIndex;
    auto const itemSizeWithSpacing = params.itemSize + 2 * params.itemSpacing;
    cursor.x += itemSizeWithSpacing.x;

    if (cursor.x + itemSizeWithSpacing.x > contentSize.x || forceNextLine) {
        // we can't fit another item
        cursor.x = 0;
        cursor.y += itemSizeWithSpacing.y;
    }
}

void DirectoryView::assignDirectory(std::filesystem::path const& dir) {
    assetsInCurrentDir.clear();
    foldersInCurrentDir.clear();
    if (dir.empty()) {
        return;
    }

    namespace fs = std::filesystem;

    for (auto&& entry: fs::directory_iterator(dir)) {
        if (utl::is_hidden(entry.path())) {
            continue;
        }
        if (fs::is_regular_file(entry.path())) {
            assetsInCurrentDir.push_back(
                assetManager->getHandleFromFile(entry.path()));
        }
        else {
            assert(fs::is_directory(entry.path()));
            foldersInCurrentDir.push_back(entry.path().stem().string());
        }
    }
}

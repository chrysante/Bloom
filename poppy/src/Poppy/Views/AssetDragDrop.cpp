#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/Views/AssetDragDrop.h"

#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/View.h"
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

bool poppy::assetButtonImpl(Editor& editor, Reference<Asset>& assetRef,
                            AssetType type, float2 size) {
    AssetManager& AM = editor.coreSystems().assetManager();
    ImGui::PushID((int)type);
    std::string label = assetRef ? assetRef->name() : "None";
    std::string popupName = utl::strcat("AssetBrowserPopup");
    auto popupID = ImGui::GetID(popupName.c_str());
    auto& g = *GImGui;
    auto& style = g.Style;
    bool haveNewAsset = false;
    if (size.x <= 0) {
        size.x = ImGui::GetContentRegionAvail().x;
    }
    if (size.y <= 0) {
        size.y = g.FontSize + 2 * style.FramePadding.y;
    }
    size.x -= size.y + style.ItemSpacing.x;
    float2 pos = ImGui::GetCursorScreenPos();
    if (ImGui::Button(label.c_str(), size)) {
        ImGui::OpenPopupEx(popupID, ImGuiPopupFlags_None);
    }
    AssetHandle handle;
    if (auto payload = acceptAssetDragDrop({ type })) {
        handle = *payload;
        haveNewAsset = true;
    }
    ImGui::SetCursorScreenPos(pos + float2(size.x + style.ItemSpacing.x, 0));
    auto icon = FontManager::get({ IconSize::_16 }, "link-ext");
    ImGui::PushFont(icon.font);
    ImGui::BeginDisabled(!assetRef);
    if (ImGui::Button(icon.utf8Rep.c_str(), { size.y, size.y })) {
        openAsset(editor, assetRef->handle());
    }
    ImGui::EndDisabled();
    ImGui::PopFont();
    ImRect bb(pos, pos + size);
    if (ImGui::BeginComboPopup(popupID, bb, ImGuiPopupFlags_None)) {
        auto assets = AM.getAssetsOfType(type);
        for (auto asset: assets) {
            bool selected = assetRef && asset == assetRef->handle();
            if (ImGui::Selectable(AM.getName(asset).c_str(), selected)) {
                handle = asset;
                haveNewAsset = true;
            }
        }
        ImGui::EndCombo();
    }
    if (handle) {
        assetRef =
            AM.get(handle, AssetRepresentation::CPU | AssetRepresentation::GPU);
    }
    ImGui::PopID();
    return haveNewAsset;
}

void poppy::openAsset(Editor& editor, bloom::AssetHandle handle) {
    auto& AM = editor.coreSystems().assetManager();
    switch (handle.type()) {
    case AssetType::Material: {
        editor.openView("Material Editor", [=, &AM](View& view) {
            view.openData(
                as<Material>(AM.get(handle, AssetRepresentation::CPU)));
        });
        break;
    }
    case AssetType::MaterialInstance: {
        editor.openView("Material Instance Viewer", [=, &AM](View& view) {
            view.openData(as<MaterialInstance>(AM.get(handle)));
        });
        break;
    }
    case AssetType::Texture2D: {
        editor.openView("Texture Viewer", [=, &AM](View& view) {
            view.openData(as<Texture2D>(AM.get(handle)));
        });
        break;
    }
    case AssetType::Scene: {
        editor.openView("Viewport");
        auto scene = as<Scene>(AM.get(handle, AssetRepresentation::CPU));
        auto& sceneSystem = editor.coreSystems().sceneSystem();
        sceneSystem.unloadAll();
        sceneSystem.loadScene(std::move(scene));
        break;
    }
    case AssetType::ScriptSource: {
        auto command = utl::strcat("open ", AM.getAbsoluteFilepath(handle));
        std::system(command.data());
        break;
    }
    default:
        Logger::Trace("Cannot open asset of type: ", handle.type());
        break;
    }
}

#define IMGUI_DEFINE_MATH_OPERATORS

#include "Poppy/Views/AssetDragDrop.h"

#include <string>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Asset/Asset.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Graphics/Texture.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewManager.h"
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

static AssetHandle displayPopup(ImGuiID popupID, ImRect bb, AssetManager& AM,
                                AssetType type, AssetHandle existingHandle) {
    if (!ImGui::BeginComboPopup(popupID, bb, ImGuiPopupFlags_None)) {
        return {};
    }
    auto assets = AM.getAssetsOfType(type);
    AssetHandle handle;
    for (auto asset: assets) {
        bool selected = asset == existingHandle;
        if (ImGui::Selectable(AM.getName(asset).c_str(), selected)) {
            handle = asset;
        }
    }
    ImGui::EndCombo();
    return handle;
}

namespace {

struct ButtonResult {
    bool pressed;
    ImRect rect;
    AssetHandle dragDropAsset;
};

} // namespace

static ButtonResult displayButtonCompact(Asset* asset, AssetType type,
                                         float2 totalSize) {
    auto& g = *GImGui;
    auto& style = g.Style;
    if (totalSize.x <= 0) {
        totalSize.x = ImGui::CalcItemWidth();
    }
    if (totalSize.y <= 0) {
        totalSize.y = g.FontSize + 2 * style.FramePadding.y;
    }
    float2 buttonSize(totalSize.x - totalSize.y, totalSize.y);
    float2 pos = ImGui::GetCursorScreenPos();
    std::string label = asset ? asset->name() : "None";
    ButtonResult result{};
    result.rect = ImRect(pos, pos + buttonSize);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    result.pressed = ImGui::ButtonEx(label.c_str(), buttonSize, 0,
                                     ImDrawFlags_RoundCornersLeft);
    if (auto payload = acceptAssetDragDrop({ type })) {
        result.dragDropAsset = *payload;
    }
    // Display open asset button
    float2 openButtonPos = pos + float2(buttonSize.x, 0);
    float2 openButtonSize(totalSize.y);
    auto icon = FontManager::get({ IconSize::_16 }, "link-ext");
    ImGui::PushFont(icon.font);
    ImGui::BeginDisabled(!asset);
    ImGui::SetCursorScreenPos(openButtonPos);
    if (ImGui::ButtonEx(icon.utf8Rep.c_str(), openButtonSize, 0,
                        ImDrawFlags_RoundCornersRight))
    {
        BL_ASSERT(asset);
        openAsset(asset->handle());
    }
    ImGui::PopStyleColor();
    ImGui::EndDisabled();
    ImGui::PopFont();
    // Draw frame
    auto* DL = ImGui::GetWindowDrawList();
    auto bordercol = ImGui::GetColorU32(ImGuiCol_Border);
    DL->AddRect(pos, pos + totalSize, bordercol, style.FrameRounding);
    return result;
}

static ButtonResult displayButton(Asset* existing, float2 size, AssetType type,
                                  AssetButtonStyle style) {
    switch (style) {
    case AssetButtonStyle::Compact:
        return displayButtonCompact(existing, type, size);
    case AssetButtonStyle::Thumbnail:
        BL_UNIMPLEMENTED();
    }
}

bool poppy::assetButtonImpl(Reference<Asset>& assetRef,
                            AssetButtonDescriptor desc) {
    AssetManager* AM = nullptr;
    Application::DispatchMessage(DispatchToken::Now, GetAssetManager{ &AM });
    BL_ASSERT(AM);
    ImGui::PushID((int)desc.type);
    std::string popupName = utl::strcat("AssetBrowserPopup");
    auto popupID = ImGui::GetID(popupName.c_str());
    AssetHandle existingHandle = assetRef ? assetRef->handle() : AssetHandle{};
    AssetHandle newHandle;
    auto buttonResult =
        displayButton(assetRef.get(), desc.size, desc.type, desc.style);
    if (buttonResult.pressed) {
        ImGui::OpenPopupEx(popupID, ImGuiPopupFlags_None);
    }
    if (buttonResult.dragDropAsset) {
        newHandle = buttonResult.dragDropAsset;
    }
    if (auto h = displayPopup(popupID, buttonResult.rect, *AM, desc.type,
                              existingHandle))
    {
        newHandle = h;
    }
    if (newHandle) {
        assetRef = AM->get(newHandle, desc.repr);
    }
    ImGui::PopID();
    return bool(newHandle);
}

template <typename AssetType>
static void openAssetInView(std::string viewName, AssetHandle handle) {
    Application::DispatchMessage(DispatchToken::NextFrame,
                                 ViewManager::OpenCommand{
                                     .name = std::move(viewName),
                                     .completion = [=](View& view) {
        view.sendData(getAsset<AssetType>(handle, AssetRepresentation::CPU));
    } });
}

void poppy::openAsset(bloom::AssetHandle handle) {
    switch (handle.type()) {
    case AssetType::Material: {
        openAssetInView<Material>("Material Editor", handle);
        break;
    }
    case AssetType::MaterialInstance: {
        openAssetInView<MaterialInstance>("Material Instance Viewer", handle);
        break;
    }
    case AssetType::Texture2D: {
        openAssetInView<Texture2D>("Texture Viewer", handle);
        break;
    }
    case AssetType::Scene: {
        openAssetInView<Scene>("Viewport", handle);
        break;
    }
    case AssetType::ScriptSource: {
        std::filesystem::path diskLocation;
        AMGetAsset msg{ .handle = handle, .diskLocation = &diskLocation };
        Application::DispatchMessage(DispatchToken::Now, msg);
        auto command = utl::strcat("open ", diskLocation);
        std::system(command.data());
        break;
    }
    default:
        Logger::Trace("Cannot open asset of type: ", handle.type());
        break;
    }
}

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
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewManager.h"
#include "Poppy/Views/DirectoryView.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

std::string poppy::selectAssetIcon(AssetType type) {
    switch (type) {
    case AssetType::StaticMesh:
        return "cube";
    case AssetType::SkeletalMesh:
        return "cube";
    case AssetType::Material:
        return "brush";
    case AssetType::MaterialInstance:
        return "delicious";
    case AssetType::Texture2D:
        return "file-image";
    case AssetType::Scene:
        return "cubes";
    case AssetType::ScriptSource:
        return "file-code";
    default:
        return "doc";
    }
}

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

static AssetHandle displayAssetListPopup(ImGuiID popupID, ImRect bb,
                                         AssetManager& AM, AssetType type,
                                         AssetHandle existingHandle) {
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

static bool displayContextMenu(ImGuiID popupID, Reference<Asset>& assetRef) {
    auto flags = ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    if (!ImGui::BeginPopupEx(popupID, flags)) {
        return false;
    }
    bool modified = false;
    if (ImGui::MenuItem("Clear")) {
        assetRef.reset();
        modified |= true;
    }
    ImGui::EndPopup();
    return modified;
}

namespace {

struct ButtonResult {
    bool clicked;
    bool rightClicked;
    ImRect rect;
    AssetHandle dragDropAsset;
};

struct PopupIDs {
    ImGuiID assetList;
    ImGuiID contextMenu;
};

} // namespace

static void displayOpenAssetButton(Asset* asset, float2 openButtonPos,
                                   float2 openButtonSize,
                                   ImDrawFlags drawFlags = 0) {
    auto icon = FontManager::get({ IconSize::_16 }, "link-ext");
    ImGui::PushFont(icon.font);
    ImGui::BeginDisabled(!asset);
    ImGui::SetCursorScreenPos(openButtonPos);
    if (ImGui::ButtonEx(icon.utf8Rep.c_str(), openButtonSize, 0, drawFlags)) {
        BL_ASSERT(asset);
        openAsset(asset->handle());
    }
    ImGui::EndDisabled();
    ImGui::PopFont();
}

static std::string getLabel(Asset const* asset) {
    return asset ? asset->name() : "None";
}

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
    ButtonResult result{};
    result.rect = ImRect(pos, pos + buttonSize);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    result.clicked = ImGui::ButtonEx(getLabel(asset).c_str(), buttonSize, 0,
                                     ImDrawFlags_RoundCornersLeft);
    result.rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (auto payload = acceptAssetDragDrop({ type })) {
        result.dragDropAsset = *payload;
    }
    displayOpenAssetButton(asset, pos + float2(buttonSize.x, 0),
                           float2(totalSize.y), ImDrawFlags_RoundCornersRight);
    ImGui::PopStyleColor();
    // Draw frame
    auto* DL = ImGui::GetWindowDrawList();
    auto bordercol = ImGui::GetColorU32(ImGuiCol_Border);
    DL->AddRect(pos, pos + totalSize, bordercol, style.FrameRounding);
    return result;
}

static bool thumbnailButtonDefault(AssetType type, float2 buttonSize) {
    auto icon =
        FontManager::get({ IconSize::_16 }, selectAssetIcon(type).c_str());
    ImGui::PushFont(icon.font);
    bool result = ImGui::ButtonEx(icon.utf8Rep.c_str(), buttonSize);
    ImGui::PopFont();
    return result;
}

static bool thumbnailButtonTexture(Texture2D const* tex, float2 buttonSize,
                                   PopupIDs popupIDs) {
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    float2 pos = ImGui::GetCursorScreenPos();
    bool clicked = ImGui::Button("###AssetButton", buttonSize);
    ImGui::PopStyleColor();
    auto* DL = ImGui::GetWindowDrawList();
    float rounding = ImGui::GetStyle().FrameRounding;
    auto col =
        ImGui::IsItemActive() ||
                ImGui::IsPopupOpen(popupIDs.assetList, ImGuiPopupFlags_None) ?
            ImGui::ColorConvertFloat4ToU32({ .8, .8, .8, 1 }) :
            IM_COL32_WHITE;
    auto handle = tex->GPUHandle;
    switch (handle.description().type) {
    case TextureType::Texture2D:
        break;
    case TextureType::CubeTexture:
        handle = createSharedTextureView(RenderAPI::Metal, handle,
                                         TextureType::Texture2D,
                                         handle.description().pixelFormat, 0, 1,
                                         0, 1);
        break;
    default:
        BL_UNIMPLEMENTED();
    }
    markObjectRenderHazard(handle);
    DL->AddImageRounded(handle.nativeHandle(), pos, pos + buttonSize, { 0, 0 },
                        { 1, 1 }, col, rounding);
    DL->AddRect(pos, pos + buttonSize, ImGui::GetColorU32(ImGuiCol_Border),
                rounding);
    return clicked;
}

static bool thumbnailButtonImpl(Asset const* asset, AssetType type,
                                float2 buttonSize, PopupIDs popupIDs) {
    switch (type) {
    case AssetType::Texture2D:
        if (auto* tex = cast<Texture2D const*>(asset); tex && tex->GPUHandle) {
            return thumbnailButtonTexture(tex, buttonSize, popupIDs);
        }
        else {
            return thumbnailButtonDefault(type, buttonSize);
        }
    default:
        return thumbnailButtonDefault(type, buttonSize);
    }
}

static void TNButtonDisplayLabelAndOpenAsset(Asset* asset, bool active,
                                             bool hovered, float2 buttonPos,
                                             float2 buttonSize) {
    auto& g = *GImGui;
    auto& style = g.Style;
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32_WHITE);
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 127));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 127));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 0, 0, 127));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0, 0, 0, 191));
    float2 padding = style.FramePadding;
    float2 openButtonSize(g.FontSize + 2 * padding.y);
    float2 barPos = buttonPos + padding;
    float2 barSize = float2(buttonSize.x - 2 * padding.x, openButtonSize.y);
    float2 labelFrameSize =
        float2(barSize.x - openButtonSize.x, openButtonSize.y);
    auto label = getLabel(asset);
    auto* DL = ImGui::GetWindowDrawList();
    float labelWidthMax = barSize.x - openButtonSize.x - 2 * padding.x;
    float labelWidthPreferred = ImGui::CalcTextSize(label.c_str()).x;
    bool isLabelClipped = labelWidthMax < labelWidthPreferred;
    float labelWidth = std::min(labelWidthPreferred, labelWidthMax);
    float2 labelPos =
        barPos + padding + float2((labelWidthMax - labelWidth) / 2, 0);
    float2 labelFramePos = barPos;
    auto frameBGCol = ImGui::GetColorU32(ImGuiCol_Button);
    auto borderCol = ImGui::GetColorU32(ImGuiCol_Border);
    float rounding = style.FrameRounding;
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    displayOpenAssetButton(asset, labelFramePos + float2(labelFrameSize.x, 0),
                           openButtonSize, ImDrawFlags_RoundCornersRight);
    ImGui::PopStyleColor();
    DL->AddRectFilled(labelFramePos, labelFramePos + labelFrameSize, frameBGCol,
                      rounding, ImDrawFlags_RoundCornersLeft);
    DL->AddRect(barPos, barPos + barSize, borderCol, rounding);
    ImGui::PushClipRect(labelPos, labelPos + float2(labelWidthMax, g.FontSize),
                        true);
    DL->AddText(nullptr, 0, labelPos, ImGui::GetColorU32(ImGuiCol_Text),
                label.c_str(), label.c_str() + label.size(), labelWidth);
    ImGui::PopClipRect();
    if (isLabelClipped && hovered &&
        ImGui::IsMouseHoveringRect(barPos, barPos + labelFrameSize))
    {
        ImGui::SetNextWindowPos(barPos);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, style.FrameRounding);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32_BLACK);
        ImGui::SetTooltip("%s", label.c_str());
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor(5);
}

static ButtonResult displayButtonThumbnail(Asset* asset, AssetType type,
                                           PopupIDs popupIDs,
                                           float2 totalSize) {
    if (totalSize.x <= 0) {
        totalSize.x = ImGui::CalcItemWidth();
    }
    if (totalSize.y <= 0) {
        totalSize.y = 75;
    }
    float2 buttonSize(std::min(totalSize.x, totalSize.y));
    float2 pos = ImGui::GetCursorScreenPos();
    ButtonResult result{};
    result.rect = ImRect(pos, pos + buttonSize);
    ImGui::SetNextItemAllowOverlap();
    result.clicked = thumbnailButtonImpl(asset, type, buttonSize, popupIDs);
    result.rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (auto payload = acceptAssetDragDrop({ type })) {
        result.dragDropAsset = *payload;
    }
    bool hovered =
        ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByItem |
                             ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bool active = ImGui::IsItemActive() ||
                  ImGui::IsPopupOpen(popupIDs.assetList, ImGuiPopupFlags_None);
    if (hovered || active) {
        TNButtonDisplayLabelAndOpenAsset(asset, active, hovered, pos,
                                         buttonSize);
    }
    return result;
}

static ButtonResult displayButton(Asset* existing, float2 size, AssetType type,
                                  AssetButtonStyle style, PopupIDs popupIDs) {
    switch (style) {
    case AssetButtonStyle::Compact:
        return displayButtonCompact(existing, type, size);
    case AssetButtonStyle::Thumbnail:
        return displayButtonThumbnail(existing, type, popupIDs, size);
    }
}

bool poppy::assetButtonImpl(char const* strID, Reference<Asset>& assetRef,
                            AssetType type, AssetButtonDescriptor const& desc) {
    AssetManager* AM = nullptr;
    Application::DispatchMessage(DispatchToken::Now, GetAssetManager{ &AM });
    BL_ASSERT(AM);
    ImGui::PushID(strID);
    PopupIDs popupIDs = { .assetList = ImGui::GetID("AssetBrowserPopup"),
                          .contextMenu = ImGui::GetID("ContextMenu") };
    AssetHandle existingHandle = assetRef ? assetRef->handle() : AssetHandle{};
    AssetHandle newHandle;
    auto buttonResult =
        displayButton(assetRef.get(), desc.size, type, desc.style, popupIDs);
    if (buttonResult.clicked) {
        ImGui::OpenPopupEx(popupIDs.assetList, ImGuiPopupFlags_None);
    }
    if (buttonResult.rightClicked) {
        ImGui::OpenPopupEx(popupIDs.contextMenu, ImGuiPopupFlags_None);
    }
    if (buttonResult.dragDropAsset) {
        newHandle = buttonResult.dragDropAsset;
    }
    if (auto h = displayAssetListPopup(popupIDs.assetList, buttonResult.rect,
                                       *AM, type, existingHandle))
    {
        newHandle = h;
    }
    if (newHandle) {
        assetRef = AM->get(newHandle, desc.repr);
    }
    bool modified = bool(newHandle);
    modified |= displayContextMenu(popupIDs.contextMenu, assetRef);
    ImGui::PopID();
    return modified;
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

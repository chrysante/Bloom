#include "Poppy/Editor/Views/DirectoryView.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <utl/filesystem_ext.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Views/AssetBrowser.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

using InteractionData = DirectoryView::InteractionData;

DirectoryView::DirectoryView(DirectoryViewDelegate delegate):
    delegate(delegate) {
    style = { .itemSpacing = 10, .itemSize = 100, .labelHeight = 20 };
}

void DirectoryView::display() {
    if (!ImGui::BeginChild("Asset Browser DirView")) {
        return;
    }
    Cursor cursor{};
    for (auto const& entry: entries) {
        displayEntry(cursor, entry);
        advanceCursor(cursor);
    }
    /// Force a line break at the end
    if (cursor.position.x > 0) {
        advanceCursor(cursor, /* forceLineBreak = */ true);
    }
    ImGui::SetCursorPos(cursor.position);
    ImGui::PushStyleColor(ImGuiCol_Separator, 0);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::EndChild();
}

void DirectoryView::openDirectory(std::filesystem::path const& dir) {
    currentDirectory = dir;
    entries.clear();
    if (dir.empty()) {
        return;
    }
    for (auto& fsEntry: std::filesystem::directory_iterator(dir)) {
        auto& path = fsEntry.path();
        DirectoryEntry entry = { path, makeUserData(fsEntry) };
        if (shallDisplay(entry)) {
            entries.push_back(makeEntryEx(entry, fsEntry));
        }
    }
    ranges::sort(entries, [](auto const& a, auto const& b) {
        if ((int)a.type < (int)b.type) {
            return true;
        }
        if ((int)a.type > (int)b.type) {
            return false;
        }
        return a.path < b.path;
    });
}

void DirectoryView::rescan() { openDirectory(currentDirectory); }

/// \Returns
static bool isDeleteEntryKey(KeyEvent event) {
#if __APPLE__
    return event.key == Key::Backspace &&
           test(event.modifierFlags & ModFlags::Super);
#endif
    return false;
}

void DirectoryView::onInput(InputEvent& event) {
    event.dispatch<InputEventMask::KeyDown>([this](KeyEvent event) {
        if (isDeleteEntryKey(event)) {
            if (interaction.selectedIndex >= 0) {
                deleteEntry(entries[interaction.selectedIndex]);
                return true;
            }
        }
        return false;
    });
}

namespace {

enum class EntryButtonState { None, Selected, Activated };

enum class RenameState { None, Apply, Cancel };

} // namespace

static auto withButtonStyle(bool selected, auto f) {
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              ImGui::GetStyleColorVec4(ImGuiCol_Button));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Button, 0);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0);
    }
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    auto result = f();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return result;
}

static EntryButtonState displayEntryButton(float2 position, float2 size,
                                           int index,
                                           InteractionData const& interaction) {
    bool pressed = withButtonStyle(interaction.selectedIndex == index, [&] {
        auto ID = generateUniqueID("entry-button", index,
                                   /* prependDoubleHash = */ true);
        ImGuiButtonFlags flags = ImGuiButtonFlags_PressedOnClick;
        ImGui::SetCursorPos(position);
        ImGui::SetNextItemAllowOverlap();
        return ImGui::ButtonEx(ID.data(), size, flags);
    });
    if (!pressed) {
        return EntryButtonState::None;
    }
    auto& g = *GImGui;
    switch (g.IO.MouseClickedCount[ImGuiMouseButton_Left]) {
    case 1:
        return EntryButtonState::Selected;
    case 2:
        return EntryButtonState::Activated;
    default:
        return EntryButtonState::None;
    }
}

static void drawIcon(IconSize iconSize, std::string iconName,
                     float2 buttonPosition, float2 buttonSize,
                     float labelHeight) {
    auto [font, iconText] = FontManager::get({ iconSize }, iconName);
    ImGui::PushFont(font);
    auto iconCursor = buttonPosition;
    iconCursor.y += (buttonSize.y - labelHeight) / 2;
    iconCursor.x += buttonSize.x / 2;
    float2 iconTextSize = ImGui::CalcTextSize(iconText.data());
    iconCursor -= iconTextSize / 2;
    ImGui::SetCursorPos(iconCursor);
    ImGui::Text("%s", iconText.data());
    ImGui::PopFont();
}

static float2 computeLabelPos(float2 labelSize, float2 buttonPos,
                              float2 buttonSize, float labelHeight) {
    buttonPos.y += buttonSize.y - labelHeight / 2;
    buttonPos.x += buttonSize.x / 2;
    buttonPos -= labelSize / 2;
    return buttonPos;
}

static void drawLabel(float2 labelSize, float2 buttonPos, float2 buttonSize,
                      float labelHeight, char const* label) {

    ImGui::SetCursorPos(
        computeLabelPos(labelSize, buttonPos, buttonSize, labelHeight));
    ImGui::Text("%s", label);
}

static RenameState drawRenameLabel(float2 labelSize, float2 buttonPos,
                                   float2 buttonSize, float labelHeight,
                                   char const* label, int index,
                                   InteractionData& interaction) {
    if (index != interaction.renamingIndex) {
        drawLabel(labelSize, buttonPos, buttonSize, labelHeight, label);
        return RenameState::None;
    }
    bool setRenameFocus = interaction.setRenameFocus;
    interaction.setRenameFocus = false;
    if (setRenameFocus) {
        ImGui::SetKeyboardFocusHere();
    }
    float2 framePadding = GImGui->Style.FramePadding;
    auto renameTextSize = ImGui::CalcTextSize(interaction.renameBuffer.data());
    ImGui::SetNextItemWidth(renameTextSize.x + 2 * framePadding.x);
    auto renamePos =
        computeLabelPos(renameTextSize, buttonPos, buttonSize, labelHeight) -
        framePadding;
    ImGui::SetCursorPos(renamePos);
    bool pressedEnter = ImGui::InputText("##rename-field",
                                         interaction.renameBuffer.data(),
                                         interaction.renameBuffer.size(),
                                         ImGuiInputTextFlags_EnterReturnsTrue);
    if (pressedEnter) {
        return RenameState::Apply;
    }
    /// If we just gained focus or have not lost focus we do nothing
    if (setRenameFocus || GImGui->LastItemData.ID == ImGui::GetActiveID()) {
        return RenameState::None;
    }
    /// We lost focus
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        return RenameState::Cancel;
    }
    return RenameState::Apply;
}

static void showPopup(int index, auto callback) {
    auto ID = generateUniqueID("entry-popup", index);
    ImGui::OpenPopupOnItemClick(ID.data());
    if (ImGui::BeginPopup(ID.data())) {
        callback();
        ImGui::EndPopup();
    }
}

static void handleDragDrop(float2 size, auto callback) {
    if (!ImGui::BeginDragDropSource()) {
        return;
    }
    if (size.x != 0.0 || size.y != 0.0) {
        ImGui::Dummy(size);
        ImGui::SetCursorPos({ 0, 0 });
    }
    callback();
    ImGui::EndDragDropSource();
}

void DirectoryView::displayEntry(Cursor const& cursor, EntryEx const& entry) {
    float2 labelSize = ImGui::CalcTextSize(entry.label.data());
    auto buttonPos = cursor.position + style.itemSpacing;
    auto iconSize = IconSize::_32;
    /// We handle all interaction first
    switch (displayEntryButton(buttonPos, style.itemSize, cursor.index,
                               interaction))
    {
    case EntryButtonState::None:
        break;
    case EntryButtonState::Selected:
        handleSelection(cursor.index);
        break;
    case EntryButtonState::Activated:
        handleActivation(entry);
        break;
    }
    showPopup(cursor.index, [&] {
        if (ImGui::Selectable("Open")) {
            handleActivation(entry);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Rename")) {
            startRenaming(entry, cursor.index);
            ImGui::CloseCurrentPopup();
        }
        if (delegate.popupMenu) {
            delegate.popupMenu(entry);
        }
    });
    handleDragDrop(style.itemSize, [&] {
        if (delegate.dragdropPayload) {
            delegate.dragdropPayload(entry);
        }
        drawIcon(iconSize, entry.icon.data(), /* position = */ { 0, 0 },
                 style.itemSize, style.labelHeight);
        drawLabel(labelSize, /* position = */ { 0, 0 }, style.itemSize,
                  style.labelHeight, entry.label.c_str());
    });
    /// We handle drawing after all interaction
    drawIcon(iconSize, entry.icon.data(), buttonPos, style.itemSize,
             style.labelHeight);
    switch (drawRenameLabel(labelSize, buttonPos, style.itemSize,
                            style.labelHeight, entry.label.c_str(),
                            cursor.index, interaction))
    {
    case RenameState::None:
        break;
    case RenameState::Apply:
        applyRenaming(entry);
        break;
    case RenameState::Cancel:
        cancelRenaming();
        break;
    }
}

void DirectoryView::advanceCursor(Cursor& cursor, bool forceNextLine) {
    ++cursor.index;
    auto itemSizeWithSpacing = style.itemSize + 2 * style.itemSpacing;
    cursor.position.x += itemSizeWithSpacing.x;
    if (forceNextLine ||
        cursor.position.x + itemSizeWithSpacing.x > ImGui::GetWindowSize().x)
    {
        cursor.position.x = 0;
        cursor.position.y += itemSizeWithSpacing.y;
    }
}

DirectoryView::EntryEx DirectoryView::makeEntryEx(
    DirectoryEntry entry,
    std::filesystem::directory_entry const& fsEntry) const {
    auto type = fsEntry.is_directory() ? EntryType::Directory : EntryType::File;
    auto path = fsEntry.path();
    std::string label = fsEntry.is_directory() ?
                            path.parent_path().filename().string() :
                            path.filename().replace_extension().string();
    return { entry, .type = type, .label = label,
             .icon = selectIconName(entry) };
}

void DirectoryView::handleSelection(int index) {
    interaction.selectedIndex = index;
}

void DirectoryView::handleActivation(EntryEx const& entry) {
    if (entry.type == EntryType::Directory) {
        if (delegate.openFolder) {
            delegate.openFolder(entry);
        }
        else {
            openDirectory(entry.path);
        }
    }
    if (entry.type == EntryType::File && delegate.openFile) {
        delegate.openFile(entry);
    }
}

void DirectoryView::startRenaming(DirectoryEntry const& entry, int index) {
    interaction.renamingIndex = index;
    auto name = entry.path.filename().string();
    auto* buf = interaction.renameBuffer.data();
    size_t bufSz = interaction.renameBuffer.size();
    std::memcpy(buf, name.data(), std::min(name.size(), bufSz));
    interaction.setRenameFocus = true;
}

void DirectoryView::applyRenaming(DirectoryEntry const& entry) {
    std::string newName = interaction.renameBuffer.data();
    cancelRenaming();
    if (newName == entry.path.filename().string()) {
        return;
    }
    if (delegate.renameEntry) {
        delegate.renameEntry(entry, newName);
        rescan();
    }
    else {
        Logger::Warn("Not renaming because no rename handler is set");
    }
}

void DirectoryView::cancelRenaming() {
    interaction.renamingIndex = -1;
    interaction.renameBuffer = {};
}

void DirectoryView::deleteEntry(DirectoryEntry const& entry) {
    if (delegate.deleteEntry) {
        delegate.deleteEntry(entry);
        rescan();
    }
    else {
        Logger::Warn("Not deleting because no deletion handler is set");
    }
}

bool DirectoryView::shallDisplay(DirectoryEntry const& entry) const {
    if (delegate.shallDisplay) {
        return delegate.shallDisplay(entry);
    }
    return true;
}

std::string DirectoryView::selectIconName(DirectoryEntry const& entry) const {
    if (std::filesystem::is_directory(entry.path)) {
        return "folder";
    }
    if (delegate.selectIcon) {
        return delegate.selectIcon(entry);
    }
    return "doc";
}

std::any DirectoryView::makeUserData(
    std::filesystem::directory_entry const& entry) const {
    if (entry.is_directory() || !delegate.makeUserData) {
        return {};
    }
    return delegate.makeUserData(entry.path());
}

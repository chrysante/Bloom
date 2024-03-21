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
using namespace mtl::short_types;
using namespace poppy;

DirectoryView::DirectoryView(DirectoryViewDelegate delegate):
    delegate(delegate) {
    params = { .itemSpacing = 10, .itemSize = 100, .labelHeight = 20 };
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

static auto generateIDs(std::string_view label, int index) {
    auto buttonID =
        generateUniqueID(label, index, /* prepentDoubleHash = */ true);
    auto popupID = generateUniqueID(buttonID.data(), index);
    return std::pair{ buttonID, popupID };
}

static bool displayEntryButton(float2 position, float2 size, int index) {
    auto ID =
        generateUniqueID("entry-button", index, /* prepentDoubleHash = */ true);
    ImGui::PushStyleColor(ImGuiCol_Button, 0);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x20FFffFF);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x20FFffFF);
    ImGuiButtonFlags flags = 0;
#if 0 // Does not work well with drag and drop
    flags |= ImGuiButtonFlags_PressedOnDoubleClick;
#endif
    ImGui::SetCursorPos(position);
    bool result = ImGui::ButtonEx(ID.data(), size, flags);
    ImGui::PopStyleColor(3);
    return result;
}

static void drawIcon(IconSize iconSize, std::string iconName,
                     float2 buttonPosition, float2 buttonSize,
                     float labelHeight) {
    ImGui::PushFont((ImFont*)icons.font(iconSize));
    auto iconCursor = buttonPosition;
    iconCursor.y += (buttonSize.y - labelHeight) / 2;
    iconCursor.x += buttonSize.x / 2;
    auto iconText = icons.unicodeStr(iconName);
    float2 iconTextSize = ImGui::CalcTextSize(iconText.data());
    iconCursor -= iconTextSize / 2;
    ImGui::SetCursorPos(iconCursor);
    ImGui::Text("%s", iconText.data());
    ImGui::PopFont();
}

static void drawLabel(float2 labelSize, float2 buttonPos, float2 buttonSize,
                      float labelHeight, char const* label) {
    buttonPos.y += buttonSize.y - labelHeight / 2;
    buttonPos.x += buttonSize.x / 2;
    buttonPos -= labelSize / 2;
    ImGui::SetCursorPos(buttonPos);
    ImGui::Text("%s", label);
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
    auto buttonPos = cursor.position + params.itemSpacing;
    auto iconSize = IconSize::_32;
    /// We handle all interaction first
    if (displayEntryButton(buttonPos, params.itemSize, cursor.index)) {
        handleOpen(entry);
    }
    showPopup(cursor.index, [&] {
        if (ImGui::Selectable("Rename")) {
            renaming = cursor.index;
            setRenameFocus = true;
            ImGui::CloseCurrentPopup();
        }
        if (delegate.popupMenu) {
            delegate.popupMenu(entry);
        }
    });
    handleDragDrop(params.itemSize, [&] {
        if (delegate.dragdropPayload) {
            delegate.dragdropPayload(entry);
        }
        drawIcon(iconSize, entry.icon.data(), /* position = */ { 0, 0 },
                 params.itemSize, params.labelHeight);
        drawLabel(labelSize, /* position = */ { 0, 0 }, params.itemSize,
                  params.labelHeight, entry.label.c_str());
    });
    /// We handle drawing after all interaction
    drawIcon(iconSize, entry.icon.data(), buttonPos, params.itemSize,
             params.labelHeight);
    drawLabel(labelSize, buttonPos, params.itemSize, params.labelHeight,
              entry.label.c_str());
}

void DirectoryView::advanceCursor(Cursor& cursor, bool forceNextLine) {
    ++cursor.index;
    auto itemSizeWithSpacing = params.itemSize + 2 * params.itemSpacing;
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

void DirectoryView::handleOpen(EntryEx const& entry) {
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

#include "Poppy/Views/DirectoryView.h"

#include <numeric>
#include <optional>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/filesystem_ext.hpp>
#include <utl/functional.hpp> // For utl::min
#include <utl/hashset.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Core/Utils.h"
#include "Poppy/Core/SelectionManager.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/Toolbar.h"

using namespace bloom;
using namespace poppy;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

struct Style {
    struct Colors {
        float4 itemInactiveSelected;
    };

    EnumArray<AppearanceType, Colors> colors = {
        /* Light: */ Colors{
            .itemInactiveSelected = { 0, 0, 0, .15 },
        },
        /* Dark: */
        Colors{
            .itemInactiveSelected = { 1, 1, 1, .15 },
        }
    };
};

static Style style;

static Style::Colors const& getStyleColors() {
    return style.colors[Appearance::get().type()];
}

class FilesystemHistory {
public:
    void push(std::filesystem::path const& path) {
        if (index + 1 < paths.size()) {
            paths.resize(index + 1);
        }
        paths.push_back(path);
        if (paths.size() != 1) {
            ++index;
        }
        BL_ASSERT(paths.size() == index + 1);
    }

    void pop() {
        if (canPop()) {
            --index;
        }
    }

    void unpop() {
        if (canUnpop()) {
            ++index;
        }
    }

    void clear() {
        paths.clear();
        index = 0;
    }

    std::filesystem::path current() const {
        BL_EXPECT(!paths.empty());
        return paths[index];
    }

    bool canPop() const { return !paths.empty() && index > 0; }

    bool canUnpop() const { return !paths.empty() && index < paths.size() - 1; }

    void displayDebugger() {
        for (auto&& [i, path]: paths | enumerate) {
            auto& p = path;
            withFont(FontDesc::UIDefault().setWeight(i == index ?
                                                         FontWeight::Semibold :
                                                         FontWeight::Regular),
                     [&] { ImGui::Text("%s", p.string().data()); });
        }
    }

private:
    std::vector<std::filesystem::path> paths;
    size_t index = 0;
};

using Rect = vml::rectangle<float>;
using SortCondition = DirectoryView::SortCondition;

struct DirectoryItemDescription {
    std::string label;
    std::string iconName;
};

struct ItemButtonState {
    bool hovered = false;
    bool held = false;
    bool selected = false;
    bool activated = false;
    bool activatedByDragDrop = false;
};

using SelectionManager = BasicSelectionManager<DirectoryEntry*>;

class ScannedDirectory {
public:
    auto entries() const {
        return _entries | transform([](auto& p) { return p.get(); });
    }

    ScannedDirectory() = default;

    void rebuild(DirectoryEntry* entry, SortCondition sortCondition) {
        /// Backup selection
        auto selectedPaths = selection() |
                             transform([](auto* s) { return s->path(); }) |
                             ranges::to<std::vector>;
        _entry = entry;
        _entries = _entry->makeChildEntries();
        sort(sortCondition);
        if (selectedPaths.empty()) {
            return;
        }
        /// Restore selection to new entries
        auto entryMap = _entries | transform([](auto& entry) {
            return std::pair{ entry->path(), entry.get() };
        }) | ranges::to<utl::hashmap<std::filesystem::path, DirectoryEntry*>>;
        _selection.clear();
        for (auto& path: selectedPaths) {
            auto itr = entryMap.find(path);
            if (itr != entryMap.end()) {
                _selection.add(itr->second);
            }
        }
    }

    void sort(SortCondition condition) {
        switch (condition) {
        case SortCondition::Name:
            ranges::sort(_entries, CompareCaseInsensitive{},
                         [](auto const& entry) {
                return entry->displayName();
            });
            break;
        case SortCondition::Extension:
            ranges::sort(_entries, CompareCaseInsensitive{},
                         [](auto const& entry) { return entry->extension(); });
            break;
        }
    }

    DirectoryEntry* find(std::filesystem::path const& path) const {
        auto e = entries();
        auto itr = ranges::find(e, path, &DirectoryEntry::path);
        return itr != e.end() ? *itr : nullptr;
    }

    DirectoryEntry* entry() const { return _entry; }
    SelectionManager& selectionWrite() {
        _selectionModified = true;
        return _selection;
    }
    SelectionManager const& selection() const { return _selection; }

    bool selectionModified() const { return _selectionModified; }

    void beginCandidates(SelectionMode mode) {
        _selection.beginCandidates(mode);
    }

    template <typename C>
    void setSelectionCandidates(C&& candidates) {
        _selection.setCandidates(std::forward<C>(candidates));
    }

private:
    friend struct poppy::DirectoryView::Impl; // For now
    DirectoryEntry* _entry;
    std::vector<std::unique_ptr<DirectoryEntry>> _entries;
    SelectionManager _selection;
    /// Tested at the end of each frame to update view layout
    bool _selectionModified = false;
};

struct DirectoryLine {
    void setRoot(std::unique_ptr<DirectoryEntry> root) {
        line.clear();
        line.emplace_back();
        line.back().rebuild(root.get(), sortCondition);
        activeIndex = 0;
        _root = std::move(root);
    }

    void rebuildToDirectory(std::filesystem::path path) {
        BL_ASSERT(std::filesystem::is_directory(path) && path.is_absolute());
        std::vector<std::filesystem::path> newLine;
        for (auto p = path;; p = p.parent_path()) {
            if (auto itr =
                    ranges::find(line, p,
                                 [](auto& s) { return s.entry()->path(); });
                itr != line.end())
            {
                activeIndex = itr - line.begin();
                line.erase(std::next(itr), line.end());
                break;
            }
            newLine.push_back(p);
            BL_ASSERT(p != p.parent_path());
        }
        for (auto& p: newLine | reverse) {
            auto* entry = line.back().find(p);
            BL_ASSERT(entry);
            line.emplace_back();
            line.back().rebuild(entry, sortCondition);
        }
        activeIndex = line.size() - 1;
    }

    bool makeCurrent(std::filesystem::path path) {
        BL_ASSERT(std::filesystem::is_directory(path) && path.is_absolute());
        bool modified = false;
        if (path != current().entry()->path()) {
            rebuildToDirectory(path);
            modified = true;
        }
        auto elem = current().selection().single();
        if (!elem) {
            return modified;
        }
        if ((*elem)->isDirectory() && activeIndex == line.size() - 1) {
            line.emplace_back();
            line.back().rebuild(*elem, sortCondition);
        }
        if (!(*elem)->isDirectory()) {
            line.erase(line.begin() + activeIndex + 1, line.end());
        }
        return modified;
    }

    void rescan() {
        DirectoryEntry* parent = root().entry();
        for (auto itr = line.begin();; ++itr) {
            auto next = std::next(itr);
            auto nextPath = next != line.end() ? next->entry()->path() :
                                                 std::filesystem::path{};
            itr->rebuild(parent, sortCondition);
            if (next == line.end()) {
                break;
            }
            parent = itr->find(nextPath);
            if (parent) {
                continue;
            }
            line.erase(next, line.end());
            activeIndex = std::min(line.size() - 1, activeIndex);
            break;
        }
    }

    bool canGoUp() const { return activeIndex > 0; }

    bool canGoDown() const {
        return current().selection().size() == 1 &&
               (*current().selection().begin())->isDirectory();
    }

    bool goUp() {
        if (!canGoUp()) {
            Logger::Error("Cannot go up, already in root directory");
            return false;
        }
        BL_ASSERT(activeIndex > 0);
        --activeIndex;
        return true;
    }

    bool goDown() {
        if (!canGoDown()) {
            Logger::Error("Cannot go down");
            return false;
        }
        makeCurrent((*current().selection().begin())->path());
        if (!current().entries().empty()) {
            current().selectionWrite().set(current().entries().front());
        }
        return true;
    }

    void setCurrentIndex(size_t index) { activeIndex = index; }

    void setSortCondition(SortCondition condition) {
        sortCondition = condition;
        for (auto& directory: line) {
            directory.sort(condition);
        }
    }

    SortCondition getSortCondition() const { return sortCondition; }

    ScannedDirectory& current() { return line[activeIndex]; }
    ScannedDirectory const& current() const { return line[activeIndex]; }
    ScannedDirectory* next() {
        return const_cast<ScannedDirectory*>(std::as_const(*this).next());
    }
    ScannedDirectory const* next() const {
        return activeIndex != line.size() - 1 ? &line[activeIndex + 1] :
                                                nullptr;
    }
    ScannedDirectory& root() { return front(); }
    ScannedDirectory const& root() const { return front(); }

    ScannedDirectory& front() { return line.front(); }
    ScannedDirectory const& front() const { return line.front(); }
    ScannedDirectory& back() { return line.back(); }
    ScannedDirectory const& back() const { return line.back(); }
    ScannedDirectory& operator[](size_t index) { return line[index]; }
    ScannedDirectory const& operator[](size_t index) const {
        return line[index];
    }
    size_t size() const { return line.size(); }
    auto begin() { return line.begin(); }
    auto begin() const { return line.begin(); }
    auto end() { return line.end(); }
    auto end() const { return line.end(); }

    void setDelegate(DirectoryViewDelegate* delegate) {
        this->delegate = delegate;
    }

private:
    DirectoryViewDelegate* delegate = nullptr;
    std::unique_ptr<DirectoryEntry> _root = nullptr;
    std::vector<ScannedDirectory> line;
    size_t activeIndex = ~size_t{};
    SortCondition sortCondition{};
};

enum class TextAlign { Left, Right, Center };

} // namespace

/// Cleared when mouse is released
struct ActiveViewData {
    bool haveSelectedItem = false;
    bool isSelectionDragging = false;
    float2 selectionRectBegin;
    ScannedDirectory* selectionRectDirectory;
};

/// Cleared at the beginning of every frame
struct FrameData {
    utl::hashmap<DirectoryEntry const*, Rect> entryBBs;
    DirectoryEntry* hoveredEntry = nullptr;
};

struct ViewData: ActiveViewData, FrameData {
    void updateEarly();
    void updateLate();

    DirectoryEntry* renamingEntry;
    bool startRenaming = false;
};

void ViewData::updateEarly() { static_cast<FrameData&>(*this) = {}; }

void ViewData::updateLate() {
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        static_cast<ActiveViewData&>(*this) = {};
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
        !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        renamingEntry = nullptr;
    }
}

using Impl = DirectoryView::Impl;

struct DirectoryView::Impl: Emitter {
    void assignCommands(CommandPalette& commandPalette);
    Toolbar makeToolbar();

    void monitorSelectionChange();
    void displayBackground(ScannedDirectory& directory);
    void displayColumns();
    void displayColumn(ScannedDirectory& directory);
    void displayColumnViewItem(ScannedDirectory& directory,
                               DirectoryEntry* entry);
    void displayPreviewColumn();
    void displayEntry(ScannedDirectory& directory, DirectoryEntry* entry,
                      Rect itemBB, Rect labelBB, Rect iconBB,
                      TextAlign textAlign, int frameDrawFlags);
    void displayEntryRenameField(DirectoryEntry const& entry, Rect labelBB);
    void toggleRenaming();
    void startRenaming(DirectoryEntry* entry);
    void entryContextMenu(ScannedDirectory const& directory,
                          DirectoryEntry* entry);
    void itemDragSource(
        ScannedDirectory const& directory,
        utl::function_view<void(DirectoryEntry const&, float2)> drawFn);
    void itemDragTarget(DirectoryEntry const& entry);
    void relocateItem(DirectoryEntry const& dest,
                      std::filesystem::path const& item);
    void beginDragSelection(ScannedDirectory& directory);
    void updateDragSelection(ScannedDirectory& directory);

    void activateEntry(DirectoryEntry& entry);
    void activateSelectedEntries();
    DirectoryItemDescription makeItemDesc(DirectoryEntry const& entry) const;

    /// Navigation

    void goUp();
    void goDown();
    void goBack();
    void goForward();
    bool navEnabled() const { return !viewData.renamingEntry; }
    void didNavigate() { viewData.renamingEntry = nullptr; }
    bool canGoUp() const;
    bool canGoDown() const;
    bool canGoBack() const;
    bool canGoForward() const;
    void makeDirectoryCurrent(std::filesystem::path const& path,
                              bool pushHistory = true);

    void moveSelection(int offset, SelectionMode mode);

    void createFolder();

    void displayDebugger();

    std::unique_ptr<DirectoryViewDelegate> delegate;
    Layout layout = Layout::Columns;
    DirectoryLine directoryLine;
    FilesystemHistory history;
    ViewData viewData;
    EnumArray<CommandType, Command const*> commands{};
    bool showDebugger = false;
};

DirectoryView::DirectoryView(): impl(std::make_unique<Impl>()) {}

DirectoryView::~DirectoryView() = default;

void DirectoryView::init(std::unique_ptr<DirectoryViewDelegate> delegate,
                         Emitter emitter, CommandPalette& commandPalette) {
    impl->delegate = std::move(delegate);
    impl->directoryLine.setDelegate(impl->delegate.get());
    impl->assignEmitter(std::move(emitter));
    impl->assignCommands(commandPalette);
}

void Impl::assignCommands(CommandPalette& commandPalette) {
    using enum CommandType;
    commands[GoUp] = commandPalette.add("Go up", { .key = Key::LeftArrow },
                                        [this] { goUp(); },
                                        [this] { return canGoUp(); });
    commands[GoDown] = commandPalette.add("Go down", { .key = Key::RightArrow },
                                          [this] { goDown(); },
                                          [this] { return canGoDown(); });
    commands[GoBack] = commandPalette.add("Go Back",
                                          { .key = Key::LeftArrow,
                                            .modFlags = ModFlags::Control },
                                          [this] { goBack(); },
                                          [this] { return canGoBack(); });
    commands[GoForward] = commandPalette.add("Go Forward",
                                             { .key = Key::RightArrow,
                                               .modFlags = ModFlags::Control },
                                             [this] { goForward(); },
                                             [this] { return canGoForward(); });
    commands[Rescan] =
        commandPalette.add("Rescan", {}, [this] { directoryLine.rescan(); });
    commands[CreateFolder] =
        commandPalette.add("Create Folder", {}, [this] { createFolder(); });
    commandPalette.add("Open", { .key = Key::O, .modFlags = ModFlags::Control },
                       [this] { activateSelectedEntries(); }, [this] {
        return !directoryLine.current().selection().empty();
    });
    commandPalette.add("Select up", { .key = Key::UpArrow },
                       [this] { moveSelection(-1, SelectionMode::Replace); });
    commandPalette.add("Select down", { .key = Key::DownArrow },
                       [this] { moveSelection(1, SelectionMode::Replace); });
    commandPalette.add("Expand selection up",
                       { .key = Key::UpArrow, .modFlags = ModFlags::Shift },
                       [this] { moveSelection(-1, SelectionMode::Toggle); });
    commandPalette.add("Expand selection down",
                       { .key = Key::DownArrow, .modFlags = ModFlags::Shift },
                       [this] { moveSelection(1, SelectionMode::Toggle); });
    commandPalette.add("Apply rename", { .key = Key::Enter },
                       [this] { toggleRenaming(); });
    commandPalette.add("Apply rename",
                       { .key = Key::D,
                         .modFlags = ModFlags::Control | ModFlags::Shift },
                       [this] { showDebugger ^= true; });
}

Toolbar DirectoryView::makeToolbar() { return impl->makeToolbar(); }

void Impl::monitorSelectionChange() {
    for (auto [index, directory]: directoryLine | enumerate) {
        if (!directory.selectionModified()) {
            continue;
        }
        viewData.renamingEntry = nullptr;
        auto toOpen = [&] {
            auto sel = directory.selection().single();
            if (sel && (*sel)->isDirectory()) {
                return (*sel)->path();
            }
            return directory.entry()->path();
        }();
        dispatch(DispatchToken::NextFrame, [=, this] {
            makeDirectoryCurrent(toOpen);
            directoryLine.setCurrentIndex(index);
        });
        directory._selectionModified = false;
        break;
    }
}

void DirectoryView::display() {
    impl->viewData.updateEarly();
    switch (impl->layout) {
    case Layout::Columns:
        impl->displayColumns();
        break;
    default:
        BL_UNIMPLEMENTED();
        break;
    }
    impl->monitorSelectionChange();
    impl->viewData.updateLate();
    if (impl->showDebugger) {
        impl->displayDebugger();
    }
}

void DirectoryView::openDirectory(std::filesystem::path directory) {
    impl->directoryLine.setRoot(impl->delegate->makeEntry(directory));
    impl->history.push(directory);
}

Command const* DirectoryView::getCommand(CommandType commandType) const {
    return impl->commands[commandType];
}

SortCondition DirectoryView::sortCondition() const {
    return impl->directoryLine.getSortCondition();
}

void DirectoryView::setSortCondition(SortCondition condition) const {
    impl->directoryLine.setSortCondition(condition);
}

Toolbar Impl::makeToolbar() {
    auto navButtonFlags = ImGuiButtonFlags_PressedOnClickRelease |
                          ImGuiButtonFlags_PressedOnDragDropHold;
    // clang-format off
    return {
        ToolbarIconButton("up-open")
            .onClick([this] { goUp(); })
            .enabled([this] { return canGoUp(); })
            .tooltip("Go up")
            .buttonFlags(navButtonFlags),
        
        ToolbarIconButton("left-open")
            .onClick([this] { goBack(); })
            .enabled([this] { return canGoBack(); })
            .tooltip("Go Back")
            .buttonFlags(navButtonFlags),
        ToolbarIconButton("right-open")
            .onClick([this] { goForward(); })
            .enabled([this] { return canGoForward(); })
            .tooltip("Go Forward")
            .buttonFlags(navButtonFlags),
        ToolbarIconButton("cw")
            .onClick([this] {
                directoryLine.rescan();
            })
            .tooltip("Rescan")
    }; // clang-format on
}

void Impl::displayBackground(ScannedDirectory& directory) {
    ImGui::SetCursorPos({ ImGui::GetScrollX(), ImGui::GetScrollY() });
    float2 size =
        (float2)ImGui::GetCursorPos() + (float2)ImGui::GetContentRegionAvail();
    ImGui::SetNextItemAllowOverlap();
    bool active = ImGui::InvisibleButton("###background-button", size);
    if (active && !viewData.hoveredEntry) {
        directory.selectionWrite().amend(std::vector<DirectoryEntry*>{});
    }
    itemDragTarget(*directory.entry());
#if 0
    backgroundContextMenu(directory);
#endif
    if (ImGui::IsItemActive() && !viewData.isSelectionDragging &&
        !viewData.hoveredEntry)
    {
        beginDragSelection(directory);
    }
    updateDragSelection(directory);
}

void Impl::displayColumns() {
    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip;
    bool hasPreviewColumn = !directoryLine.back().selection().empty();
    int numColumns = (int)directoryLine.size() + hasPreviewColumn;

    if (!ImGui::BeginTable("column-view", numColumns, tableFlags)) {
        return;
    }
    // Setup
    float defaultWidth = 240;
    for (auto [index, directory]: directoryLine | enumerate) {
        ImGui::TableSetupColumn(directory.entry()->path().c_str(),
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    if (hasPreviewColumn) {
        ImGui::TableSetupColumn("##preview-column",
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    // Display
    for (auto&& [index, directory]: directoryLine | enumerate) {
        ImGui::TableNextColumn();
        displayColumn(directory);
    }
    if (hasPreviewColumn) {
        ImGui::TableNextColumn();
        displayPreviewColumn();
    }
    ImGui::EndTable();
}

static float2 contentSizeWithoutScrollbar() {
    float2 windowSize = ImGui::GetContentRegionAvail();
    auto* window = ImGui::GetCurrentWindowRead();
    if (window->ScrollbarX) {
        windowSize.y =
            window->ClipRect.GetHeight() - 2 * ImGui::GetStyle().FramePadding.y;
    }
    return windowSize;
}

void Impl::displayColumn(ScannedDirectory& directory) {
    if (&directoryLine.front() == &directory) {
        ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    auto clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
    ImGui::BeginChild(directory.entry()->path().c_str(),
                      contentSizeWithoutScrollbar());
    ImGui::PopStyleVar();
    ImGui::PushClipRect(clipRect.Min, clipRect.Max, false);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         ImGui::GetStyle().WindowPadding.y);
    for (auto* entry: directory.entries()) {
        displayColumnViewItem(directory, entry);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         ImGui::GetStyle().WindowPadding.y);
    displayBackground(directory);
    ImGui::PopClipRect();
    ImGui::EndChild();
}

void Impl::displayColumnViewItem(ScannedDirectory& directory,
                                 DirectoryEntry* entry) {
    auto& style = ImGui::GetStyle();
    auto* window = ImGui::GetCurrentWindow();
    float itemHeight = ImGui::GetFontSize() + 2 * style.FramePadding.y;
    float itemWidth = ImGui::GetWindowWidth() - style.ScrollbarSize;
    float2 itemSize = float2(itemWidth, itemHeight);
    float2 framePadding = style.FramePadding;
    float2 cursorPos = window->DC.CursorPos;
    auto itemBB = Rect::from_size(cursorPos, itemSize);
    auto iconBB = Rect::from_size(cursorPos + framePadding + float2(3, 0),
                                  float2(itemHeight) - 2 * framePadding);
    auto labelBB = Rect(float2(iconBB.upper_bound().x + framePadding.x,
                               iconBB.lower_bound().y),
                        float2(itemBB.upper_bound() - framePadding));
    bool selected = directory.selection().contains(entry);
    ImDrawFlags frameDrawFlags = 0;
    if (selected) {
        //        bool prevSelected =
        //            entry != directory.entries().front() &&
        //            directory.selection().contains(directory.entries[entryIndex
        //            - 1]);
        //        bool nextSelected =
        //            entryIndex < directory.entries.size() - 1 &&
        //            directory.selection.contains(directory.entries[entryIndex
        //            + 1]);
        //        if (prevSelected && nextSelected) {
        //            frameDrawFlags = ImDrawFlags_RoundCornersNone;
        //        }
        //        else if (prevSelected) {
        //            frameDrawFlags = ImDrawFlags_RoundCornersBottom;
        //        }
        //        else if (nextSelected) {
        //            frameDrawFlags = ImDrawFlags_RoundCornersTop;
        //        }
        frameDrawFlags = ImDrawFlags_RoundCornersTop |
                         ImDrawFlags_RoundCornersBottom;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        (float2)style.FramePadding + float2(0, 2));
    displayEntry(directory, entry, itemBB, labelBB, iconBB, TextAlign::Left,
                 frameDrawFlags);
    ImGui::PopStyleVar(2);
    entryContextMenu(directory, entry);
}

void Impl::displayPreviewColumn() {
    ImGui::BeginChild("preview-column", contentSizeWithoutScrollbar());
    auto& dir = directoryLine.current();
    if (dir.selection().empty()) {
        displayEmptyWithError("No entry selected");
    }
    else if (dir.selection().size() == 1) {
        auto* entry = *dir.selection().begin();
        if (entry->isDirectory()) {
            displayEmptyWithError("Folder preview should not show up");
        }
        else if (!entry->displayPreview()) {
            displayEmptyWithReason("No Preview");
        }
    }
    else {
        displayEmptyWithReason(
            utl::strcat(dir.selection().size(), " Objects Selected"));
    }
    ImGui::EndChild();
}

static void drawEntryFrame(Rect BB, ImU32 color, ImDrawFlags drawFlags) {
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRectFilled(BB.lower_bound(), BB.upper_bound(), color,
                      ImGui::GetStyle().FrameRounding, drawFlags);
}

static void drawEntry(Rect labelBB, Rect iconBB, TextAlign align,
                      std::string const& label, std::string const& icon,
                      ImU32 textColor, bool renaming) {
    auto* DL = ImGui::GetWindowDrawList();
    /// Icon
    if (auto iconDesc = FontManager::get({ IconSize::_16 }, std::string(icon));
        iconDesc.font)
    {
        float size = iconBB.size().fold(utl::min);
        float2 iconTextSize =
            iconDesc.font->CalcTextSizeA(size, FLT_MAX, -1,
                                         iconDesc.utf8Rep.c_str());
        float2 iconPosition =
            iconBB.lower_bound() + (iconBB.size() - iconTextSize) / 2;
        DL->AddText(iconDesc.font, size, iconPosition, textColor,
                    iconDesc.utf8Rep.c_str());
    }
    if (renaming) {
        return;
    }
    /// Label
    auto const calcLabelPos = [&labelBB, align](float2 labelSize) {
        switch (align) {
        case TextAlign::Left:
            return labelBB.lower_bound();
        case TextAlign::Right:
            return labelBB.lower_bound() +
                   float2(labelBB.size().x - labelSize.x, 0);
        case TextAlign::Center:
            return labelBB.lower_bound() +
                   float2((labelBB.size().x - labelSize.x) / 2, 0);
        }
    };
    if (float2 labelSize =
            ImGui::CalcTextSize(label.data(), label.data() + label.size());
        labelSize.x <= labelBB.size().x)
    {
        DL->AddText(nullptr, 0, calcLabelPos(labelSize), textColor,
                    label.data(), label.data() + label.size());
    }
    else {
        size_t endPos = label.size();
        while (endPos > 0 &&
               ImGui::CalcTextSize(label.data(), label.data() + endPos).x >
                   labelBB.size().x)
        {
            --endPos;
        }
        std::string labelStr(label.data(), label.data() + endPos);
        for (auto&& [i, c]: labelStr | reverse | enumerate) {
            if (i == 3) {
                break;
            }
            c = '.';
        }
        labelSize = ImGui::CalcTextSize(labelStr.data(),
                                        labelStr.data() + labelStr.size());
        DL->AddText(nullptr, 0, calcLabelPos(labelSize), textColor,
                    labelStr.data(), labelStr.data() + labelStr.size());
    }
}

void Impl::displayEntry(ScannedDirectory& directory, DirectoryEntry* entry,
                        Rect itemBB, Rect labelBB, Rect iconBB,
                        TextAlign textAlign, int frameDrawFlags) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }
    auto itemDesc = makeItemDesc(*entry);
    bool entrySelected = directory.selection().contains(entry);
    auto& g = *GImGui;
    auto& style = g.Style;
    if (viewData.selectionRectDirectory &&
        directory.entry() == viewData.selectionRectDirectory->entry())
    {
        viewData.entryBBs[entry] = itemBB;
    }
    /// Button behaviour
    ItemButtonState buttonState = [&] {
        ImRect BB = ImRect(itemBB.lower_bound(), itemBB.upper_bound());
        auto displayName = entry->displayName();
        ImGuiID ID = window->GetID(displayName.data(),
                                   displayName.data() + displayName.size());
        ItemButtonState buttonState{};
        ImGui::ItemSize(itemBB.size(), style.FramePadding.y);
        if (!ImGui::ItemAdd(BB, ID)) {
            return buttonState;
        }
        auto buttonFlags = ImGuiButtonFlags_PressedOnDoubleClick |
                           ImGuiButtonFlags_PressedOnDragDropHold |
                           ImGuiButtonFlags_PressedOnClick |
                           ImGuiButtonFlags_PressedOnRelease |
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight;
        bool buttonActive = ImGui::ButtonBehavior(BB, ID, &buttonState.hovered,
                                                  &buttonState.held,
                                                  buttonFlags);
        if (!entry->isDirectory() && g.DragDropActive) {
            buttonState.hovered = false;
        }
        if (!buttonActive) {
            return buttonState;
        }
        bool wantSelectEntry = false;
        SelectionMode selectionMode = getSelectionMode();
        if (!entrySelected) {
            wantSelectEntry |= ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        }
        else if (!viewData.haveSelectedItem && !g.DragDropActive) {
            wantSelectEntry |= ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        }
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            wantSelectEntry = true;
            if (selectionMode == SelectionMode::Replace &&
                directory.selection().contains(entry))
            {
                selectionMode = SelectionMode::Add;
            }
        }
        if (wantSelectEntry) {
            buttonState.selected =
                directory.selectionWrite().amend(entry, selectionMode);
            viewData.haveSelectedItem = true;
        }
        buttonState.activated =
            ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 2;
        if (buttonState.activated) {
            activateEntry(*entry);
        }
        buttonState.activatedByDragDrop =
            !g.IO.MouseReleased[ImGuiMouseButton_Left] &&
            !buttonState.selected && !buttonState.activated;
        return buttonState;
    }();
    if (buttonState.activated) {
        if (!entry->isDirectory()) {
            entry->open();
        }
    }
    if (ImGui::IsItemHovered()) {
        viewData.hoveredEntry = entry;
    }
    itemDragSource(directory,
                   [&](DirectoryEntry const& draggingItem, float2 position) {
        /// Preview drawing Code
        ImGui::SetCursorPos(position - (float2)ImGui::GetWindowPos());
        ImGui::ItemSize(itemBB.size(), style.FramePadding.y);
        auto ddItemDesc = makeItemDesc(draggingItem);
        auto localItemBB = Rect::from_size(position, itemBB.size());
        auto localLabelBB = Rect::from_size(position - itemBB.lower_bound() +
                                                labelBB.lower_bound(),
                                            labelBB.size());
        auto localIconBB = Rect::from_size(position - itemBB.lower_bound() +
                                               iconBB.lower_bound(),
                                           iconBB.size());
        drawEntryFrame(localItemBB, ImGui::GetColorU32(ImGuiCol_Header),
                       ImDrawFlags_None);
        drawEntry(localLabelBB, localIconBB, textAlign, ddItemDesc.label,
                  ddItemDesc.iconName, IM_COL32_WHITE, false);
    });
    itemDragTarget(*entry);
    // Draw Frame
    auto [frameColor, textColor] = [&]() -> std::pair<ImU32, ImU32> {
        std::pair<ImU32, ImU32> const noFrame = { 0, ImGui::GetColorU32(
                                                         ImGuiCol_Text) };
        std::pair<ImU32, ImU32> const selectedActive = {
            ImGui::GetColorU32(ImGuiCol_Header), IM_COL32_WHITE
        };
        std::pair<ImU32, ImU32> const selectedInactive = {
            ImGui::ColorConvertFloat4ToU32(
                getStyleColors().itemInactiveSelected),
            ImGui::GetColorU32(ImGuiCol_Text)
        };
        if (!entrySelected) {
            return noFrame;
        }
        if (&directory == &directoryLine.current()) {
            return selectedActive;
        }
        return selectedInactive;
    }();
    if (entrySelected) {
        drawEntryFrame(itemBB, frameColor, frameDrawFlags);
    }
    bool isRenamingEntry = viewData.renamingEntry == entry;
    if (isRenamingEntry) {
        displayEntryRenameField(*entry, labelBB);
    }
    drawEntry(labelBB, iconBB, textAlign, itemDesc.label, itemDesc.iconName,
              textColor, isRenamingEntry);
}

static std::array<char, 512> RenameBuffer{};

void Impl::displayEntryRenameField(DirectoryEntry const& entry, Rect labelBB) {
    if (viewData.startRenaming) {
        std::string displayName(entry.displayName());
        if (displayName.size() >= RenameBuffer.size()) {
            Logger::Error("Cannot rename file: name exceeds buffer size");
            viewData.renamingEntry = nullptr;
            return;
        }
        ImGui::SetKeyboardFocusHere();
        std::strncpy(RenameBuffer.data(), displayName.c_str(),
                     RenameBuffer.size());
    }
    float2 stashedCursor = ImGui::GetCursorPos();
    ImGui::SetCursorScreenPos(labelBB.lower_bound() -
                              (float2)ImGui::GetStyle().FramePadding);
    ImGui::SetNextItemWidth(labelBB.size().x);
    ImGui::InputText("###Rename field", RenameBuffer.data(),
                     RenameBuffer.size(), ImGuiInputTextFlags_AutoSelectAll);
    ImGui::SetCursorPos(stashedCursor);
    viewData.startRenaming = false;
}

void Impl::toggleRenaming() {
    if (viewData.renamingEntry) {
        auto& entry = *viewData.renamingEntry;
        entry.rename(std::string(RenameBuffer.data()));
        viewData.renamingEntry = nullptr;
        dispatch(DispatchToken::NextFrame, [this] { directoryLine.rescan(); });
    }
    else if (directoryLine.current().selection().size() == 1) {
        startRenaming(*directoryLine.current().selection().begin());
    }
}

void Impl::startRenaming(DirectoryEntry* entry) {
    viewData.renamingEntry = entry;
    viewData.startRenaming = true;
}

void Impl::entryContextMenu(ScannedDirectory const& directory,
                            DirectoryEntry* entry) {
    std::string popupID = utl::strcat("EntryPopup-", entry->path());
    ImGui::OpenPopupOnItemClick(popupID.c_str());
    if (ImGui::BeginPopup(popupID.c_str())) {
        if (!entry->isDirectory() && ImGui::MenuItem("Open")) {
            activateSelectedEntries();
        }
        if (directory.selection().size() == 1 && ImGui::MenuItem("Rename")) {
            startRenaming(entry);
        }
        if (ImGui::MenuItem("Delete")) {
            for (auto* s: directory.selection()) {
                s->erase();
            }
            dispatch(DispatchToken::NextFrame,
                     [this] { directoryLine.rescan(); });
        }
        ImGui::Separator();
        entry->contextMenu();
        ImGui::EndPopup();
    }
}

static void writeUserDataSize(auto out, uint32_t size) {
    for (int i = 0; i < 4; ++i, ++out) {
        *out = ((char*)&size)[i];
    }
}

static uint32_t readUserDataSize(auto itr) {
    uint32_t size = 0;
    for (int i = 0; i < 4; ++i, ++itr) {
        ((char*)&size)[i] = *itr;
    }
    return size;
}

static std::vector<char> packDragDropPayload(
    SelectionManager const& selection) {
    std::vector<char> result;
    for (auto& entry: selection) {
        auto path = entry->path().string();
        result.insert(result.end(), path.begin(), path.end());
        result.push_back('\0');
        auto data = entry->imguiDragDropUserData();
        uint32_t size = utl::narrow_cast<uint32_t>(data.size());
        writeUserDataSize(std::back_inserter(result), size);
        result.insert(result.end(), data.begin(), data.end());
    }
    return result;
}

/// Paths and user data
using DragDropDelivery = std::vector<DirectoryViewDragDropItem>;

static DragDropDelivery unpackDragDropPayload(char const* data, size_t size) {
    DragDropDelivery result;
    size_t j = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] != '\0') {
            continue;
        }
        result.push_back({ std::filesystem::path(data + j, data + i), {} });
        ++i;
        uint32_t size = readUserDataSize(data + i);
        i += 4;
        result.back().userData =
            utl::small_vector<char>(data + i, data + i + size);
        i += size;
        j = i;
    }
    return result;
}

std::vector<DirectoryViewDragDropItem> poppy::peekFileDragDropData() {
    auto* payload = ImGui::GetDragDropPayload();
    return unpackDragDropPayload((char const*)payload->Data, payload->DataSize);
}

void Impl::itemDragSource(
    ScannedDirectory const& directory,
    utl::function_view<void(DirectoryEntry const&, float2)> drawFn) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, 0x0);
    if (ImGui::BeginDragDropSource()) {
        bool havePayload = ImGui::GetDragDropPayload() &&
                           ImGui::GetDragDropPayload()->Data;
        if (!havePayload) {
            auto payload = packDragDropPayload(directory.selection());
            ImGui::SetDragDropPayload(delegate->imguiDragDropType(),
                                      payload.data(), payload.size(),
                                      ImGuiCond_Once);
        }
        if (auto* payload = ImGui::GetDragDropPayload();
            payload && std::string_view(payload->DataType) ==
                           delegate->imguiDragDropType())
        {
            float2 pos = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto delivery = unpackDragDropPayload((char const*)payload->Data,
                                                  payload->DataSize);
            for (auto [index, item]: delivery | enumerate) {
                auto entry = delegate->makeEntry(item.path);
                drawFn(*entry, pos + index * float2(10, 10));
            }
        }
        ImGui::EndDragDropSource();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void Impl::itemDragTarget(DirectoryEntry const& entry) {
    if (!entry.isDirectory()) {
        return;
    }
    DragDropDelivery delivery;
    if (auto* payload = ImGui::GetDragDropPayload();
        payload && payload->IsDataType(delegate->imguiDragDropType()))
    {
        delivery = unpackDragDropPayload((char const*)payload->Data,
                                         payload->DataSize);
        if (delivery.empty()) {
            return;
        }
        /// Not drop into ourself
        if (ranges::contains(delivery, entry.path(),
                             &DirectoryViewDragDropItem::path))
        {
            return;
        }
        /// Not drop into parent directory
        if (delivery.front().path.parent_path() == entry.path()) {
            return;
        }
    }
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }
    ImGuiPayload const* payload =
        ImGui::AcceptDragDropPayload(delegate->imguiDragDropType());
    if (!payload) {
        ImGui::EndDragDropTarget();
        return;
    }
    dispatch(DispatchToken::NextFrame, [=, &entry, this] {
        bool movedAny = false;
        for (auto& [path, userData]: delivery) {
            if (!path.has_parent_path() || path.parent_path() == entry.path()) {
                continue;
            }
            relocateItem(entry, path);
            movedAny = true;
        }
        if (movedAny) {
            directoryLine.rescan();
        }
    });
    ImGui::EndDragDropTarget();
}

void Impl::relocateItem(DirectoryEntry const& dest,
                        std::filesystem::path const& item) {
    BL_ASSERT(dest.isDirectory());
    auto entry = delegate->makeEntry(item);
    entry->relocate(dest.path() / item.filename());
}

void Impl::beginDragSelection(ScannedDirectory& directory) {
    viewData.isSelectionDragging = true;
    viewData.selectionRectBegin = (float2)ImGui::GetMousePos();
    viewData.selectionRectDirectory = &directory;
    directory.beginCandidates(getSelectionMode());
}

void Impl::updateDragSelection(ScannedDirectory& dir) {
    if (viewData.selectionRectDirectory != &dir) {
        return;
    }
    auto& g = *GImGui;
    auto& window = *g.CurrentWindow;
    float2 pos1 = viewData.selectionRectBegin;
    float2 pos2 = ImGui::GetMousePos();
    Rect rect(pos1, pos2);
    SelectionManager::SetType candidates;
    for (auto* entry: dir.entries()) {
        if (vml::do_intersect(rect, viewData.entryBBs[entry])) {
            candidates.insert(entry);
        }
    }
    dir.setSelectionCandidates(std::move(candidates));
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        dir.selectionWrite().applyCandidates();
    }
    ImRect windowRect = window.Rect();
    float2 min = vml::max(rect.lower_bound(), (float2)windowRect.Min);
    float2 max = vml::min(rect.upper_bound(), (float2)windowRect.Max);
    float4 rectColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
    rectColor.a *= 0.5f;
    auto* DL = window.DrawList;
    DL->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(rectColor));
    DL->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border));
}

void Impl::activateEntry(DirectoryEntry& entry) {
    if (!entry.isDirectory()) {
        entry.open();
    }
}

void Impl::activateSelectedEntries() {
    for (auto& entry: directoryLine.current().selection()) {
        activateEntry(*entry);
    }
}

DirectoryItemDescription Impl::makeItemDesc(DirectoryEntry const& entry) const {
    return { .label = std::string(entry.displayName()),
             .iconName = entry.iconName() };
}

void Impl::goUp() {
    if (navEnabled() && directoryLine.goUp()) {
        history.push(directoryLine.current().entry()->path());
        didNavigate();
    }
}

void Impl::goDown() {
    if (navEnabled() && directoryLine.goDown()) {
        history.push(directoryLine.current().entry()->path());
        didNavigate();
    }
}

void Impl::goBack() {
    if (navEnabled() && history.canPop()) {
        history.pop();
        makeDirectoryCurrent(history.current(), /*pushHistory: */ false);
        didNavigate();
    }
}

void Impl::goForward() {
    if (navEnabled() && history.canUnpop()) {
        history.unpop();
        makeDirectoryCurrent(history.current(), /*pushHistory: */ false);
        didNavigate();
    }
}

bool Impl::canGoUp() const { return directoryLine.canGoUp(); }

bool Impl::canGoDown() const { return directoryLine.canGoDown(); }

bool Impl::canGoBack() const { return history.canPop(); }

bool Impl::canGoForward() const { return history.canUnpop(); }

void Impl::makeDirectoryCurrent(std::filesystem::path const& path,
                                bool pushHistory) {
    //    if (directoryLine.current().entry()->path() == path) {
    //        return;
    //    }
    if (directoryLine.makeCurrent(path)) {
        if (pushHistory) {
            history.push(path);
        }
    }
}

void Impl::moveSelection(int offset, SelectionMode mode) {
    if (!navEnabled()) {
        return;
    }
    //    auto& dir = directoryLine.current();
    //    BL_ASSERT(mode != SelectionMode::Add);
    //    if (dir.selection().empty()) {
    //        return;
    //    }
    //    auto last = dir.selection().last();
    //    BL_ASSERT(last);
    //    auto itr = ranges::find(dir.entries(), *last);
    //    BL_ASSERT(itr != dir.entries().end());
    //    auto index = itr - dir.entries().begin() + offset;
    //    if (index >= 0 && index < (ssize_t)dir.entries().size()) {
    //        dir.selection.amend(dir.entries[index], mode);
    //        dir.selectionModified = true;
    //    }
}

void Impl::createFolder() {
    auto& dir = directoryLine.current();
    dir.entry()->createFolder("New Folder");
    dispatch(DispatchToken::NextFrame, [&] { directoryLine.rescan(); });
    //    auto entry = dir.find(name);
    //    if (!entry) {
    //        Logger::Error(
    //            "Internal error occurred. Failed to locate newly created
    //            folder");
    //        return;
    //    }
    //    startRenaming(*entry);
}

void Impl::displayDebugger() {
    ImGui::Begin("DirViewDebugger");
    ImGui::Text("Current directory: %s",
                directoryLine.current().entry()->path().c_str());
    ImGui::Text("Hovered entry: %s", viewData.hoveredEntry ?
                                         viewData.hoveredEntry->path().c_str() :
                                         "NULL");
    if (viewData.isSelectionDragging) {
        ImGui::SeparatorText("Selection dragging");
        ImGui::Text("selectionRectBegin: %f, %f",
                    (double)viewData.selectionRectBegin.x,
                    (double)viewData.selectionRectBegin.y);
        ImGui::Text("selectionRectDirectory: %s",
                    viewData.selectionRectDirectory ?
                        viewData.selectionRectDirectory->entry()
                            ->path()
                            .c_str() :
                        "NULL");
    }
    ImGui::Text("Rename Buffer: %s", RenameBuffer.data());
    if (viewData.renamingEntry) {
        ImGui::Text("renamingEntry: %s",
                    viewData.renamingEntry->path().c_str());
    }
    ImGui::SeparatorText("Directory Line");
    for (auto& dir: directoryLine) {
        if (ImGui::TreeNodeEx(dir.entry()->path().c_str(),
                              ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::TreeNode("Entries")) {
                for (auto* entry: dir.entries()) {
                    ImGui::Text("%s", entry->path().c_str());
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Selection", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (auto& entry: dir.selection()) {
                    ImGui::Text("%s", entry->path().c_str());
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
    ImGui::SeparatorText("History");
    history.displayDebugger();
    ImGui::End();
}

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
#include <utl/stack.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/ResourceUtil.h"
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

DeleteConfirmResponse const DeleteConfirmResponse::MayDelete;

DeleteConfirmResponse DeleteConfirmResponse::ConfirmNumItems(size_t numItems) {
    return DeleteConfirmResponse(
        false, utl::strcat("Are you sure you want to delete the seleted ",
                           numItems, " item", numItems != 1 ? "s" : "", "?"));
}

DeleteConfirmResponse DeleteConfirmResponse::Confirm(std::string message) {
    return DeleteConfirmResponse(false, std::move(message));
}

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

class NavEntryStack {
public:
    auto const& get() const { return stack; }

    void push(DirectoryEntry* entry) {
        auto& c = stack.container();
        auto& s = selection();
        for (auto itr = c.begin(); itr != c.end();) {
            if (*itr == entry || !s.contains(*itr)) {
                itr = c.erase(itr);
            }
            else {
                ++itr;
            }
        }
        if (s.contains(entry)) {
            stack.push(entry);
        }
    }

    void pop() {
        if (!stack.empty()) {
            stack.pop();
        }
    }

    DirectoryEntry* top() const {
        return stack.empty() ? nullptr : stack.top();
    }

    void clear() { stack.clear(); }

    auto begin() const { return stack.begin(); }
    auto end() const { return stack.end(); }

private:
    SelectionManager const& selection() const;

    utl::stack<DirectoryEntry*> stack;
};

/// We inherit privately from `NavEntryStack` so we can access the selection
/// manager in `NavEntryStack` by casting `this` to `ScannedDirectory*`
class ScannedDirectory: private NavEntryStack {
public:
    auto entries() const {
        return _entries | transform([](auto& p) { return p.get(); });
    }

    ScannedDirectory() = default;

    static std::vector<std::filesystem::path> backupPaths(auto&& range) {
        return range | transform([](auto* entry) { return entry->path(); }) |
               ranges::to<std::vector>;
    }

    void rebuild(DirectoryEntry* entry, SortCondition sortCondition) {
        /// Backup selection
        auto selBackup = backupPaths(selection());
        auto navBackup = backupPaths(navStack());
        _entry = entry;
        _entries = _entry->makeChildEntries();
        sort(sortCondition);
        if (selBackup.empty() && navBackup.empty()) {
            return;
        }
        /// Restore selection and nav stack to new entries
        auto entryMap = _entries | transform([](auto& entry) {
            return std::pair{ entry->path(), entry.get() };
        }) | ranges::to<utl::hashmap<std::filesystem::path, DirectoryEntry*>>;
        _selection.clear();
        for (auto& path: selBackup) {
            auto itr = entryMap.find(path);
            if (itr != entryMap.end()) {
                _selection.add(itr->second);
            }
        }
        navStack().clear();
        for (auto& path: navBackup) {
            auto itr = entryMap.find(path);
            if (itr != entryMap.end()) {
                navStack().push(itr->second);
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

    size_t getIndex(DirectoryEntry const* entry) const {
        auto itr = ranges::find(_entries, entry, ToAddress<>);
        return utl::narrow_cast<size_t>(itr - _entries.begin());
    }

    DirectoryEntry* entry() const { return _entry; }
    DirectoryEntry* entryAt(size_t index) const {
        return _entries[index].get();
    }
    bool isSelected(size_t index) const {
        return selection().contains(entryAt(index));
    }
    DirectoryEntry* navEntry() const { return navStack().top(); }
    void pushNavEntry(DirectoryEntry* entry) { navStack().push(entry); }
    NavEntryStack const& getNavEntryStack() const { return navStack(); }

    SelectionManager const& selection() const { return _selection; }

    void clearSelection(bool setModified = true) {
        _selection.clear();
        navStack().clear();
        _selectionModified = setModified;
    }

    bool amendSelection(DirectoryEntry* entry, SelectionMode mode) {
        _selectionModified = true;
        bool result = _selection.amend(entry, mode);
        navStack().push(entry);
        return result;
    }

    bool selectionModified() const { return _selectionModified; }

    void beginSelectionCandidates(SelectionMode mode) {
        _selection.beginCandidates(mode);
    }

    template <typename C>
    void setSelectionCandidates(C&& candidates) {
        _selection.setCandidates(std::forward<C>(candidates));
    }

    void applySelectionCandidates() {
        _selection.applyCandidates();
        _selectionModified = true;
    }

    size_t size() const { return entries().size(); }
    bool empty() const { return entries().empty(); }
    DirectoryEntry* front() const { return entries().front(); }
    DirectoryEntry* back() const { return entries().back(); }

private:
    friend struct DirectoryView::Impl; // For now
    friend class NavEntryStack;        // To cast to derived

    NavEntryStack& navStack() { return *this; }
    NavEntryStack const& navStack() const { return *this; }

    DirectoryEntry* _entry;
    std::vector<std::unique_ptr<DirectoryEntry>> _entries;
    SelectionManager _selection;
    /// Tested at the end of each frame to update view layout
    bool _selectionModified = false;
};

SelectionManager const& NavEntryStack::selection() const {
    return static_cast<ScannedDirectory const&>(*this).selection();
}

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
        auto single = current().selection().single();
        if (!single || !single->isDirectory()) {
            line.erase(line.begin() + activeIndex + 1, line.end());
            return modified;
        }
        if (activeIndex == line.size() - 1) {
            line.emplace_back();
            line.back().rebuild(single, sortCondition);
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
        current().clearSelection(false);
        --activeIndex;
        return true;
    }

    bool goDown() {
        if (!canGoDown()) {
            Logger::Error("Cannot go down");
            return false;
        }
        makeCurrent((*current().selection().begin())->path());
        if (current().selection().empty() && !current().entries().empty()) {
            current().amendSelection(current().entries().front(),
                                     SelectionMode::Replace);
        }
        if (current().entries().empty()) {
            --activeIndex;
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

    bool isValid() const { return _root != nullptr; }

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

static constexpr auto DirectoryPopupName = "DirectoryPopup";

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
    DirectoryEntry* hoveredEntryBelowActive = nullptr;
    bool focused = false;
};

struct ViewData: ActiveViewData, FrameData {
    void updateEarly();
    void updateLate();

    ScannedDirectory* popupDirectory = nullptr;
    DirectoryEntry* renamingEntry;
    bool startRenaming = false;
    bool didNavigate = false;
};

void ViewData::updateEarly() {
    static_cast<FrameData&>(*this) = {};
    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
}

void ViewData::updateLate() {
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        static_cast<ActiveViewData&>(*this) = {};
    }
    if ((ImGui::IsKeyPressed(ImGuiKey_Escape) ||
         !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) &&
        !startRenaming)
    {
        renamingEntry = nullptr;
    }
    startRenaming = false;
    didNavigate = false;
}

using Impl = DirectoryView::Impl;

struct DirectoryView::Impl: Emitter {
    void assignCommands(CommandPalette& commandPalette);
    Toolbar makeToolbar();

    void monitorSelectionChange();
    void display();
    void displayBackground(ScannedDirectory& directory);
    void displayColumns();
    void displayColumn(ScannedDirectory& directory);
    void displayColumnViewItem(ScannedDirectory& directory,
                               DirectoryEntry* entry, size_t index);
    void displayPreviewColumn();
    void displayEntry(ScannedDirectory& directory, DirectoryEntry* entry,
                      Rect itemBB, Rect labelBB, Rect iconBB,
                      TextAlign textAlign, int frameDrawFlags);
    void displayEntryRenameField(DirectoryEntry const& entry, Rect labelBB);
    void toggleRenaming();
    void startRenaming(DirectoryEntry* entry);
    void entryContextMenu(ScannedDirectory& directory, DirectoryEntry* entry);
    void itemDragSource(
        ScannedDirectory const& directory,
        utl::function_view<void(DirectoryEntry const&, float2)> drawFn);
    bool itemDragTarget(DirectoryEntry const& entry, bool hasTargetColor);
    void relocateItem(DirectoryEntry const& dest,
                      std::filesystem::path const& item);
    void beginDragSelection(ScannedDirectory& directory);
    void updateDragSelection(ScannedDirectory& directory);
    void displayDirectoryPopup();
    void displayDeleteValidationPopup();

    void activateEntry(DirectoryEntry& entry);
    void activateSelectedEntries();
    void deleteSelectedEntries(ScannedDirectory& dir);
    DirectoryItemDescription makeItemDesc(DirectoryEntry const& entry) const;

    /// Navigation

    void goUp();
    void goDown();
    void goBack();
    void goForward();
    bool navEnabled() const { return !viewData.renamingEntry; }
    void didNavigate() {
        viewData.renamingEntry = nullptr;
        viewData.didNavigate = true;
    }
    bool canGoUp() const;
    bool canGoDown() const;
    bool canGoBack() const;
    bool canGoForward() const;
    void makeDirectoryCurrent(std::filesystem::path const& path,
                              bool pushHistory = true);

    void moveSelection(int offset, SelectionMode mode);

    void createFolder(ScannedDirectory& dir);

    void displayDebugger();

    std::unique_ptr<DirectoryViewDelegate> delegate;
    Layout layout = Layout::Columns;
    DirectoryLine directoryLine;
    FilesystemHistory history;
    ViewData viewData;
    EnumArray<CommandType, Command const*> commands{};
    std::function<void()> deleteContinuation;
    DeleteConfirmResponse deleteConfirmResponse;
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
    commands[CreateFolder] = commandPalette.add("Create Folder", {}, [this] {
        createFolder(directoryLine.current());
    });
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
    commandPalette.add("Delete selected",
                       { .key = Key::Backspace, .modFlags = ModFlags::Control },
                       [this] {
        deleteSelectedEntries(directoryLine.current());
    }, [this] { return !viewData.renamingEntry; });
    commandPalette.add("Show Debugger",
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
        if (!viewData.startRenaming) {
            viewData.renamingEntry = nullptr;
        }
        auto toOpen = [&] {
            auto sel = directory.selection().single();
            if (sel && sel->isDirectory()) {
                return sel->path();
            }
            return directory.entry()->path();
        }();
        dispatch(DispatchToken::NextFrame, [=, this] {
            makeDirectoryCurrent(toOpen);
            directoryLine.setCurrentIndex(index);
            viewData.didNavigate = true;
        });
        directory._selectionModified = false;
        break;
    }
}

void DirectoryView::display() { impl->display(); }

void DirectoryView::openDirectory(std::filesystem::path directory) {
    impl->directoryLine.setRoot(impl->delegate->makeEntry(directory));
    impl->history.push(directory);
}

std::filesystem::path DirectoryView::currentDirectory() const {
    if (!impl->directoryLine.isValid()) {
        return {};
    }
    return impl->directoryLine.current().entry()->path();
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

void Impl::display() {
    if (!directoryLine.isValid()) {
        ImGui::Dummy({});
        return;
    }
    ImGui::BeginChild("DirectoryView");
    viewData.updateEarly();
    switch (layout) {
    case Layout::Columns:
        displayColumns();
        break;
    default:
        BL_UNIMPLEMENTED();
        break;
    }
    displayDeleteValidationPopup();
    monitorSelectionChange();
    viewData.updateLate();
    ImGui::EndChild();
    if (showDebugger) {
        displayDebugger();
    }
}

void Impl::displayBackground(ScannedDirectory& directory) {
    ImGui::SetCursorPos({ ImGui::GetScrollX(), ImGui::GetScrollY() });
    float2 size =
        (float2)ImGui::GetCursorPos() + (float2)ImGui::GetContentRegionAvail();
    ImGui::SetNextItemAllowOverlap();
    bool active = ImGui::InvisibleButton("###background-button", size);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup(DirectoryPopupName);
        viewData.popupDirectory = &directory;
    }
    displayDirectoryPopup();
    if (active && !viewData.hoveredEntry &&
        getSelectionMode() == SelectionMode::Replace)
    {
        directory.clearSelection();
    }
    itemDragTarget(*directory.entry(), true);
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
    bool havePreviewColumn = &directoryLine.current() ==
                                 &directoryLine.back() &&
                             !directoryLine.current().selection().empty();
    int numColumns = (int)directoryLine.size() + havePreviewColumn;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 0, 0 });
    bool open = ImGui::BeginTable("column-view", numColumns, tableFlags);
    ImGui::PopStyleVar();
    if (!open) {
        return;
    }
    // Setup
    float defaultWidth = 240;
    for (auto [index, directory]: directoryLine | enumerate) {
        ImGui::TableSetupColumn(directory.entry()->path().c_str(),
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    if (havePreviewColumn) {
        ImGui::TableSetupColumn("##preview-column",
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    // Display
    for (auto& dir: directoryLine) {
        ImGui::TableNextColumn();
        displayColumn(dir);
        if (viewData.didNavigate && &dir == &directoryLine.current()) {
            ImGui::SetScrollHereX();
        }
    }
    if (havePreviewColumn) {
        ImGui::TableNextColumn();
        displayPreviewColumn();
        if (viewData.didNavigate) {
            ImGui::SetScrollHereX(1);
        }
    }
    ImGui::EndTable();
}

/// To reduce window height if we have horizontal scroll
static float2 contentSizeWithoutScrollbar() {
    float2 windowSize = ImGui::GetContentRegionAvail();
    auto* window = ImGui::GetCurrentWindowRead();
    if (window->ScrollbarX) {
        windowSize.y =
            window->ClipRect.GetHeight() - 2 * ImGui::GetStyle().FramePadding.y;
    }
    return windowSize;
}

static bool beginColumnChild(char const* ID, int flags = 0) {
    auto clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    bool open = ImGui::BeginChild(ID, contentSizeWithoutScrollbar(), flags);
    ImGui::PopStyleVar();
    ImGui::PushClipRect(clipRect.Min, clipRect.Max, false);
    return open;
}
static void endColumnChild() {
    ImGui::PopClipRect();
    ImGui::EndChild();
}

void Impl::displayColumn(ScannedDirectory& dir) {
    beginColumnChild(dir.entry()->path().c_str());
    bool isCurrent = &directoryLine.current() == &dir;
    if (viewData.focused && isCurrent &&
        !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        ImGui::FocusWindow(ImGui::GetCurrentWindowRead());
    }
    ImGui::Dummy({ 0, ImGui::GetStyle().WindowPadding.y });
    auto windowRect = [&] {
        auto r = ImGui::GetCurrentWindowRead()->ClipRect;
        return Rect{ (float2)r.Min, (float2)r.Max };
    }();
    for (auto [index, entry]: dir.entries() | enumerate) {
        displayColumnViewItem(dir, entry, index);
        if (isCurrent && viewData.didNavigate && dir.navEntry() == entry &&
            !vml::encloses(windowRect, viewData.entryBBs[entry]))
        {
            ImGui::SetScrollHereY();
        }
    }
    ImGui::Dummy({ 0, ImGui::GetStyle().WindowPadding.y });
    displayBackground(dir);
    endColumnChild();
}

void Impl::displayColumnViewItem(ScannedDirectory& directory,
                                 DirectoryEntry* entry, size_t index) {
    auto& style = ImGui::GetStyle();
    auto* window = ImGui::GetCurrentWindow();
    float itemHeight = ImGui::GetFontSize() + 2 * style.FramePadding.y;
    float itemWidth = ImGui::GetWindowWidth() - 2 * style.WindowPadding.x;
    float2 itemSize = float2(itemWidth, itemHeight);
    float2 framePadding = style.FramePadding;
    float2 cursorPos =
        (float2)window->DC.CursorPos + float2(style.WindowPadding.x, 0);
    auto itemBB = Rect::from_size(cursorPos, itemSize);
    auto iconBB = Rect::from_size(cursorPos + framePadding + float2(3, 0),
                                  float2(itemHeight) - 2 * framePadding);
    auto labelBB = Rect(float2(iconBB.upper_bound().x + framePadding.x,
                               iconBB.lower_bound().y),
                        float2(itemBB.upper_bound() - framePadding));
    bool selected = directory.selection().contains(entry);
    ImDrawFlags frameDrawFlags = 0;
    bool prevSelected = false;
    bool nextSelected = false;
    if (selected) {
        prevSelected = index > 0 && directory.isSelected(index - 1);
        nextSelected = index < directory.entries().size() - 1 &&
                       directory.isSelected(index + 1);
        if (prevSelected && nextSelected) {
            frameDrawFlags = ImDrawFlags_RoundCornersNone;
        }
        else if (prevSelected) {
            frameDrawFlags = ImDrawFlags_RoundCornersBottom;
        }
        else if (nextSelected) {
            frameDrawFlags = ImDrawFlags_RoundCornersTop;
        }
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        (float2)style.FramePadding + float2(0, 2));
    displayEntry(directory, entry, itemBB, labelBB, iconBB, TextAlign::Left,
                 frameDrawFlags);
    ImGui::PopStyleVar(2);
    if (selected && nextSelected) {
        auto* DL = ImGui::GetWindowDrawList();
        float2 begin = cursorPos + float2(framePadding.x, itemSize.y - 1);
        float2 end = begin + float2(itemSize.x - 2 * framePadding.x, 0);
        DL->AddLine(begin, end,
                    ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 0.33 }));
    }
    entryContextMenu(directory, entry);
}

void Impl::displayPreviewColumn() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                        { ImGui::GetStyle().WindowPadding.x, 0 });
    beginColumnChild("###PreviewColumn",
                     ImGuiChildFlags_AlwaysUseWindowPadding);
    ImGui::PopStyleVar();
    auto& dir = directoryLine.current();
    if (dir.selection().size() == 1) {
        auto* entry = *dir.selection().begin();
        if (!entry->isDirectory() && !entry->displayPreview()) {
            displayEmptyWithReason("No Preview");
        }
    }
    else {
        displayEmptyWithReason(
            utl::strcat(dir.selection().size(), " Objects Selected"));
    }
    endColumnChild();
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
    viewData.entryBBs[entry] = itemBB;
    /// Button behaviour
    [&] {
        ImRect BB = ImRect(itemBB.lower_bound(), itemBB.upper_bound());
        auto displayName = entry->displayName();
        ImGuiID ID = window->GetID(displayName.data(),
                                   displayName.data() + displayName.size());
        ImGui::ItemSize(itemBB.size(), style.FramePadding.y);
        if (!ImGui::ItemAdd(BB, ID)) {
            return;
        }
        auto buttonFlags = ImGuiButtonFlags_PressedOnDoubleClick |
                           ImGuiButtonFlags_PressedOnDragDropHold |
                           ImGuiButtonFlags_PressedOnClick |
                           ImGuiButtonFlags_PressedOnRelease |
                           ImGuiButtonFlags_MouseButtonLeft |
                           ImGuiButtonFlags_MouseButtonRight;
        bool hovered = false;
        bool held = false;
        bool buttonActive =
            ImGui::ButtonBehavior(BB, ID, &hovered, &held, buttonFlags);
        if (!entry->isDirectory() && g.DragDropActive) {
            hovered = false;
        }
        if (!buttonActive) {
            return;
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
        bool selected = false;
        if (wantSelectEntry) {
            selected = directory.amendSelection(entry, selectionMode);
            viewData.haveSelectedItem = true;
        }
        bool activated = ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) ==
                         2;
        if (activated) {
            activateEntry(*entry);
        }
        bool activatedByDragDrop = !g.IO.MouseReleased[ImGuiMouseButton_Left] &&
                                   !selected && !activated;
        (void)activatedByDragDrop; // TODO: Use this variable
    }();
    if (ImGui::IsItemHovered()) {
        viewData.hoveredEntry = entry;
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
        viewData.hoveredEntryBelowActive = entry;
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
    bool isTarget = itemDragTarget(*entry, false);
    // Draw Frame
    auto [frameColor, textColor] = [&]() -> std::pair<ImU32, ImU32> {
        std::pair<ImU32, ImU32> noFrame = { 0,
                                            ImGui::GetColorU32(ImGuiCol_Text) };
        std::pair<ImU32, ImU32> selectedActive = {
            ImGui::GetColorU32(ImGuiCol_Header), IM_COL32_WHITE
        };
        std::pair<ImU32, ImU32> selectedInactive = {
            ImGui::ColorConvertFloat4ToU32(
                getStyleColors().itemInactiveSelected),
            ImGui::GetColorU32(ImGuiCol_Text)
        };
        if (isTarget) {
            return selectedActive;
        }
        if (!entrySelected) {
            return noFrame;
        }
        if (viewData.focused && &directory == &directoryLine.current()) {
            return selectedActive;
        }
        return selectedInactive;
    }();
    if (entrySelected || isTarget) {
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
    float2 padding = { 2, 0 };
    float rounding = 2;
    ImGui::SetCursorScreenPos(labelBB.lower_bound() - padding);
    ImGui::SetNextItemWidth(labelBB.size().x);
    auto& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_ChildBg]);
    ImGui::InputText("###Rename field", RenameBuffer.data(),
                     RenameBuffer.size(), ImGuiInputTextFlags_AutoSelectAll);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::SetCursorPos(stashedCursor);
}

void Impl::toggleRenaming() {
    if (viewData.renamingEntry) {
        auto* entry = viewData.renamingEntry;
        viewData.renamingEntry = nullptr;
        std::string newName = RenameBuffer.data();
        RenameBuffer = {};
        if (entry->displayName() != newName) {
            dispatch(DispatchToken::NextFrame, [=, this] {
                entry->rename(newName);
                directoryLine.rescan();
            });
        }
    }
    else if (directoryLine.current().selection().size() == 1) {
        startRenaming(*directoryLine.current().selection().begin());
    }
}

void Impl::startRenaming(DirectoryEntry* entry) {
    viewData.renamingEntry = entry;
    viewData.startRenaming = true;
}

#ifdef __APPLE__
#define FILE_EXPLORER_NAME "Finder"
#else
#define FILE_EXPLORER_NAME "Explorer"
#endif

void Impl::entryContextMenu(ScannedDirectory& dir, DirectoryEntry* entry) {
    std::string popupID = utl::strcat("EntryPopup-", entry->path());
    ImGui::OpenPopupOnItemClick(popupID.c_str());
    if (ImGui::BeginPopup(popupID.c_str())) {
        if (!entry->isDirectory() && ImGui::MenuItem("Open")) {
            activateSelectedEntries();
        }
        if (ImGui::MenuItem("Reveal in " FILE_EXPLORER_NAME)) {
            showFileInFileExplorer((*dir.selection().begin())->path());
        }
        if (dir.selection().size() == 1 && ImGui::MenuItem("Rename")) {
            dispatch(DispatchToken::NextFrame,
                     [=, this] { startRenaming(entry); });
        }
        if (ImGui::MenuItem("Delete")) {
            dispatch(DispatchToken::NextFrame,
                     [this, &dir] { deleteSelectedEntries(dir); });
        }
        ImGui::Separator();
        entry->contextMenu();
        ImGui::EndPopup();
    }
}

void Impl::deleteSelectedEntries(ScannedDirectory& dir) {
    if (dir.selection().empty()) {
        return;
    }
    auto selectedIndex = [&]() -> std::optional<size_t> {
        auto single = dir.selection().single();
        if (!single) {
            return std::nullopt;
        }
        return dir.getIndex(single);
    }();
    auto action = [=, this, &dir] {
        for (auto* s: dir.selection()) {
            s->erase();
        }
        directoryLine.rescan();
        if (selectedIndex) {
            auto* entry = dir.entryAt(std::min(*selectedIndex, dir.size() - 1));
            dir.amendSelection(entry, SelectionMode::Replace);
        }
    };
    auto entries = dir.selection() | ToSmallVector<>;
    deleteConfirmResponse = delegate->confirmDeletion(entries);
    if (deleteConfirmResponse.mayDelete()) {
        action();
    }
    else {
        deleteContinuation = [=, this] {
            dispatch(DispatchToken::NextFrame, action);
        };
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

bool Impl::itemDragTarget(DirectoryEntry const& entry, bool hasTargetColor) {
    if (!entry.isDirectory()) {
        return false;
    }
    DragDropDelivery delivery;
    if (auto* payload = ImGui::GetDragDropPayload();
        payload && payload->IsDataType(delegate->imguiDragDropType()))
    {
        delivery = unpackDragDropPayload((char const*)payload->Data,
                                         payload->DataSize);
        if (delivery.empty()) {
            return false;
        }
        /// Not drop into ourself
        if (ranges::contains(delivery, entry.path(),
                             &DirectoryViewDragDropItem::path))
        {
            return false;
        }
        /// Not drop into parent directory
        if (delivery.front().path.parent_path() == entry.path()) {
            return false;
        }
    }
    auto col = hasTargetColor ? ImGui::GetColorU32(ImGuiCol_Header) : 0;
    ImGui::PushStyleColor(ImGuiCol_DragDropTarget, col);
    utl::scope_guard guard = [] { ImGui::PopStyleColor(); };
    bool isTarget = ImGui::BeginDragDropTarget();
    if (!isTarget) {
        return false;
    }
    ImGuiPayload const* payload =
        ImGui::AcceptDragDropPayload(delegate->imguiDragDropType());
    if (!payload) {
        ImGui::EndDragDropTarget();
        return true;
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
    return true;
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
    directory.beginSelectionCandidates(getSelectionMode());
    dispatch(DispatchToken::NextFrame, [this, &directory] {
        makeDirectoryCurrent(directory.entry()->path());
    });
}

void Impl::updateDragSelection(ScannedDirectory& dir) {
    if (viewData.selectionRectDirectory != &dir) {
        return;
    }
    auto& g = *GImGui;
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
    dir.pushNavEntry(viewData.hoveredEntryBelowActive);
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        dir.applySelectionCandidates();
    }
    if (layout != Layout::Columns) {
        auto& window = *g.CurrentWindow;
        ImRect windowRect = window.Rect();
        float2 min = vml::max(rect.lower_bound(), (float2)windowRect.Min);
        float2 max = vml::min(rect.upper_bound(), (float2)windowRect.Max);
        float4 rectColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
        rectColor.a *= 0.5f;
        auto* DL = window.DrawList;
        DL->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(rectColor));
        DL->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border));
    }
}

void Impl::displayDirectoryPopup() {
    if (!ImGui::BeginPopup(DirectoryPopupName)) {
        viewData.popupDirectory = nullptr;
        return;
    }
    BL_ASSERT(viewData.popupDirectory);
    auto& dir = *viewData.popupDirectory;
    if (ImGui::MenuItem("New Folder")) {
        dispatch(DispatchToken::NextFrame, [this, &dir] { createFolder(dir); });
    }
    ImGui::EndPopup();
}

void Impl::displayDeleteValidationPopup() {
    static constexpr auto DeleteValidationPopupName = "DeleteValidationPopup";
    if (!deleteConfirmResponse.mayDelete()) {
        ImGui::OpenPopup(DeleteValidationPopupName);
    }
    ImGui::SetNextWindowSize({ 300, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 15, 15 });
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 15, 15 });
    utl::scope_guard guard = [&] { ImGui::PopStyleVar(2); };
    if (!ImGui::BeginPopupModal(DeleteValidationPopupName, nullptr,
                                ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoResize))
    {
        return;
    }
    float2 windowSize = ImGui::GetWindowSize();
    auto& message = deleteConfirmResponse.message();
    ImGui::PushFont(
        FontManager::get(FontDesc::UIDefault().setWeight(FontWeight::Bold)));
    ImGui::TextWrapped("%s", message.c_str());
    ImGui::PopFont();
    auto& style = ImGui::GetStyle();
    float2 padding = style.FramePadding;
    float2 spacing = style.ItemSpacing;
    float confirmWidth =
        ImGui::CalcTextSize("Confirm Delete").x + 2 * padding.x;
    float cancelWidth = ImGui::CalcTextSize("Cancel").x + 2 * padding.x;
    float controlWidth = confirmWidth + cancelWidth + spacing.x;
    float controlPos = (windowSize.x - controlWidth) / 2;
    ImGui::SetCursorPosX(controlPos);
    bool done = false;
    if (ImGui::Button("Confirm Delete")) {
        if (deleteContinuation) {
            deleteContinuation();
        }
        done = true;
    }
    ImGui::SameLine();
    ImGui::SetItemDefaultFocus();
    if (ImGui::Button("Cancel")) {
        done = true;
    }
    if (done) {
        deleteContinuation = {};
        deleteConfirmResponse = DeleteConfirmResponse::MayDelete;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
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
    if (!directoryLine.makeCurrent(path)) {
        return;
    }
    if (pushHistory) {
        history.push(path);
    }
}

static void amendNavEntry(ScannedDirectory& dir, auto indexValid, int modIndex,
                          int navIndex, int offset) {
    auto& s = dir.selection();
    if (navIndex != modIndex) {
        for (int i = modIndex + offset; indexValid(i); i += offset) {
            auto* e = dir.entryAt(i);
            if (!s.contains(e)) {
                break;
            }
            dir.pushNavEntry(e);
        }
    }
    else {
        for (int i = modIndex + offset; indexValid(i); i += offset) {
            auto* e = dir.entryAt(i);
            if (s.contains(e)) {
                dir.pushNavEntry(e);
                break;
            }
        }
    }
}

void Impl::moveSelection(int offset, SelectionMode mode) {
    BL_ASSERT(offset == 1 || offset == -1);
    if (!navEnabled()) {
        return;
    }
    auto& dir = directoryLine.current();
    BL_ASSERT(mode != SelectionMode::Add);
    auto* navEntry = [&]() -> DirectoryEntry* {
        if (auto* n = dir.navEntry()) {
            return n;
        }
        BL_ASSERT(dir.selection().empty());
        if (dir.empty()) {
            return nullptr;
        }
        return offset > 0 ? dir.front() : dir.back();
    }();
    if (!navEntry) {
        return;
    }
    auto& s = dir.selection();
    if (!s.contains(navEntry)) {
        dir.amendSelection(navEntry, mode);
        return;
    }
    auto indexValid = [size = (int)dir.entries().size()](int i) {
        return i >= 0 && i < size;
    };
    auto isSelected = [&](int index) {
        return s.contains(dir.entryAt((size_t)index));
    };
    int navIndex = utl::narrow_cast<int>(dir.getIndex(navEntry));
    int modIndex = [&] {
        bool nextSelected = indexValid(navIndex + offset) &&
                            isSelected(navIndex + offset);
        bool prevSelected = indexValid(navIndex - offset) &&
                            isSelected(navIndex - offset);
        if (nextSelected && !prevSelected && mode == SelectionMode::Toggle) {
            return navIndex;
        }
        return navIndex + offset;
    }();
    if (!indexValid(modIndex)) {
        return;
    }
    dir.amendSelection(dir.entryAt((size_t)modIndex), mode);
    amendNavEntry(dir, indexValid, modIndex, navIndex, offset);
}

void Impl::createFolder(ScannedDirectory& dir) {
    auto baseName = "New Folder";
    std::string name = baseName;
    int counter = 2;
    while (ranges::contains(dir.entries(), name, &DirectoryEntry::displayName))
    {
        name = utl::strcat(baseName, counter++);
    }
    dir.entry()->createFolder(name);
    directoryLine.rescan();
    auto* entry = dir.find(dir.entry()->path() / name);
    if (!entry) {
        Logger::Error(
            "Failed to start renaming. Newly created folder does not exist");
        return;
    }
    startRenaming(entry);
    dir.amendSelection(entry, SelectionMode::Replace);
}

static void debugItemImpl(std::string_view name, auto const&... args) {
    ImGui::TextEx(utl::strcat(name, ": ", args...).c_str());
}

static void debugItem(std::string_view name, char const* str) {
    debugItemImpl(name, str);
}

static void debugItem(std::string_view name, DirectoryEntry const* entry) {
    debugItemImpl(name, entry ? entry->path() : "NULL");
}

static void debugItem(std::string_view name, ScannedDirectory const* dir) {
    debugItemImpl(name, dir ? dir->entry() : nullptr);
}

static void debugItem(std::string_view name, float2 f) {
    debugItemImpl(name, f);
}

void Impl::displayDebugger() {
    ImGui::Begin("DirViewDebugger");
    debugItem("Current directory", directoryLine.current().entry());
    debugItem("hoveredEntry", viewData.hoveredEntry);
    debugItem("hoveredEntryBelowActive", viewData.hoveredEntryBelowActive);
    debugItem("selectionRectBegin", viewData.selectionRectBegin);
    debugItem("selectionRectDirectory", viewData.selectionRectDirectory);
    debugItem("RenameBuffer", RenameBuffer.data());
    debugItem("renamingEntry", viewData.renamingEntry);
    debugItem("popupDirectory", viewData.popupDirectory);
    ImGui::SeparatorText("Directory Line");
    for (auto& dir: directoryLine) {
        auto font = FontDesc::UIDefault();
        if (&dir == &directoryLine.current()) {
            font.setWeight(FontWeight::Bold);
        }
        ImGui::PushFont(FontManager::get(font));
        bool open = ImGui::TreeNodeEx(dir.entry()->path().c_str(),
                                      ImGuiTreeNodeFlags_DefaultOpen);
        ImGui::PopFont();
        if (!open) {
            continue;
        }
        if (ImGui::TreeNodeEx("navEntryStack", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (auto* entry: dir.getNavEntryStack().get() | reverse) {
                debugItem("", entry);
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto* entry: dir.selection()) {
                debugItem("", entry);
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Entries")) {
            for (auto* entry: dir.entries()) {
                debugItem("", entry);
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
    ImGui::SeparatorText("History");
    history.displayDebugger();
    ImGui::End();
}

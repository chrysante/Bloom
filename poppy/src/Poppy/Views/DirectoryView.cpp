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

struct DirEntryHash {
    using is_transparent = void;

    size_t operator()(DirectoryEntry const& entry) const {
        return utl::hash<std::filesystem::path>{}(entry.path());
    }
};

struct DirEntryEqual {
    using is_transparent = void;

    bool operator()(std::filesystem::directory_entry const& a,
                    std::filesystem::directory_entry const& b) const {
        return a == b;
    }
};

using SelectionManager =
    BasicSelectionManager<DirectoryEntry, DirEntryHash, DirEntryEqual>;

struct CaseInsensitiveLess {
    bool operator()(auto&& a, auto&& b) {
        return std::lexicographical_compare(ranges::begin(a), ranges::end(a),
                                            ranges::begin(b), ranges::end(b),
                                            [](auto x, auto y) {
            return std::tolower((int)x) < std::tolower((int)y);
        });
    }
};

struct CaseInsensitiveEqual {
    bool operator()(auto&& a, auto&& b) {
        return std::equal(ranges::begin(a), ranges::end(a), ranges::begin(b),
                          ranges::end(b), [](auto x, auto y) {
            return std::tolower((int)x) == std::tolower((int)y);
        });
    }
};

struct ScannedDirectory {
    static ScannedDirectory Build(DirectoryViewDelegate& delegate,
                                  SortCondition condition,
                                  std::filesystem::path path) {
        ScannedDirectory result;
        result.path = path;
        result.rescan(delegate, condition);
        return result;
    }

    bool rescan(DirectoryViewDelegate& delegate, SortCondition condition) {
        entries.clear();
        std::error_code err;
        auto itr = std::filesystem::directory_iterator(path, err);
        if (err) {
            return false;
        }
        for (auto entry: itr) {
            if (!utl::is_hidden(entry.path())) {
                entries.push_back({ entry, delegate.makeUserData(entry) });
            }
        }
        sort(condition);
        auto set = entries | ranges::to<SelectionManager::SetType>;
        /// Erase all selected elements that don't exist enymore
        auto toErase = selection | filter([&](auto& entry) {
            return !set.contains(entry);
        }) | ToSmallVector<>;
        ranges::for_each(toErase, [&](auto& entry) { selection.erase(entry); });
        return true;
    }

    void sort(SortCondition condition) {
        switch (condition) {
        case SortCondition::Name:
            ranges::sort(entries, CaseInsensitiveLess{},
                         [](auto& a) { return a.path().filename().string(); });
            break;
        case SortCondition::Extension:
            ranges::sort(entries, [&](auto& a, auto& b) {
                auto aExt = a.path().extension().string();
                auto bExt = b.path().extension().string();
                if (CaseInsensitiveEqual{}(aExt, bExt)) {
                    return CaseInsensitiveLess{}(a.path().filename().string(),
                                                 b.path().filename().string());
                }
                return CaseInsensitiveLess{}(aExt, bExt);
            });
            break;
        }
    }

    /// \Returns the directory entry matching \p filename if it exists
    std::optional<DirectoryEntry> find(std::filesystem::path const& filename) {
        auto itr = ranges::find(entries, filename, [](auto& entry) {
            return entry.path().filename();
        });
        return itr != entries.end() ? std::optional(*itr) : std::nullopt;
    }

    std::filesystem::path path;
    std::vector<DirectoryEntry> entries;
    SelectionManager selection;
    /// Tested at the end of each frame to update view layout
    bool selectionModified = false;

private:
    ScannedDirectory() = default;
};

struct DirectoryLine {
    void setRoot(std::filesystem::path const& directory) {
        _root = directory;
        line.clear();
        line.push_back(
            ScannedDirectory::Build(*delegate, sortCondition, directory));
        activeIndex = 0;
    }

    bool makeCurrent(std::filesystem::path leafDirectory) {
        if (!std::filesystem::is_directory(leafDirectory)) {
            Logger::Error("Path is not a Directory: ", leafDirectory);
            return false;
        }
        std::vector<std::filesystem::path> newLine;
        for (auto current = leafDirectory; current != _root;
             current = current.parent_path())
        {
            if (current == current.parent_path()) {
                Logger::Error("Failed to scan directory");
                return false;
            }
            newLine.push_back(current);
        }
        newLine.push_back(_root);
        auto map = line | transform([](auto& dir) {
            return std::pair{ dir.path, std::move(dir) };
        }) | ranges::to<utl::hashmap<std::filesystem::path, ScannedDirectory>>;
        line.clear();
        for (auto& path: newLine | reverse) {
            if (auto itr = map.find(path); itr != map.end()) {
                line.push_back(std::move(itr->second));
                map.erase(itr);
            }
            else {
                line.push_back(
                    ScannedDirectory::Build(*delegate, sortCondition, path));
            }
        }
        for (auto [line, next]: zip(line, line | drop(1))) {
            std::filesystem::directory_entry entry(next.path);
            line.selection.set(
                DirectoryEntry(entry, delegate->makeUserData(entry)));
        }
        activeIndex = size() - 1;
        return true;
    }

    void rescan() {
        for (auto itr = line.begin(); itr != line.end(); ++itr) {
            if (!itr->rescan(*delegate, sortCondition)) {
                line.erase(itr, line.end());
                activeIndex = std::min(line.size() - 1, activeIndex);
                break;
            }
        }
    }

    bool canGoUp() const { return activeIndex > 0; }

    bool canGoDown() const {
        return current().selection.size() == 1 &&
               current().selection.begin()->is_directory();
    }

    bool goUp() {
        if (!canGoUp()) {
            Logger::Error("Cannot go up, already in root directory");
            return false;
        }
        line.pop_back();
        BL_ASSERT(activeIndex > 0);
        --activeIndex;
        return true;
    }

    bool goDown() {
        if (!canGoDown()) {
            Logger::Error("Cannot go down");
            return false;
        }
        makeCurrent(current().selection.begin()->path());
        if (!current().entries.empty()) {
            current().selection.set(current().entries.front());
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
    std::filesystem::path _root;
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
    size_t selectionRectDirectoryIndex;
};

/// Cleared at the beginning of every frame
struct FrameData {
    utl::hashmap<std::filesystem::path, Rect> entryBBs;
    std::filesystem::path hoveredEntry;
};

struct ViewData: ActiveViewData, FrameData {
    void updateEarly() { static_cast<FrameData&>(*this) = {}; }
    void updateLate() {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
            ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            static_cast<ActiveViewData&>(*this) = {};
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        {
            renamingEntry = std::nullopt;
        }
    }
    std::optional<DirectoryEntry> renamingEntry;
    bool startRenaming = false;
};

using Impl = DirectoryView::Impl;

struct DirectoryView::Impl: Emitter {
    void assignCommands(CommandPalette& commandPalette);
    Toolbar makeToolbar();

    void displayBackground(ScannedDirectory& directory, size_t dirIndex);
    void displayColumns();
    void displayColumn(ScannedDirectory& directory, size_t lineIndex);
    void displayColumnViewItem(ScannedDirectory& directory,
                               size_t directoryIndex, size_t entryIndex);
    void displayPreviewColumn();
    void displayEntry(ScannedDirectory& directory, size_t directoryIndex,
                      size_t entryIndex, Rect itemBB, Rect labelBB, Rect iconBB,
                      TextAlign textAlign, int frameDrawFlags);
    void displayEntryRenameField(DirectoryEntry const& entry, Rect labelBB);
    void toggleRenaming();
    void startRenaming(DirectoryEntry const& entry);
    void entryContextMenu(DirectoryEntry const& entry,
                          ScannedDirectory const& directory);
    void itemDragSource(
        ScannedDirectory const& directory,
        utl::function_view<void(DirectoryEntry const&, float2)> drawFn);
    void itemDragTarget(DirectoryEntry const& entry);
    void beginDragSelection(size_t directoryIndex);
    void updateDragSelection(size_t directoryIndex);

    void activateEntry(DirectoryEntry const& entry);
    void activateSelectedEntries();
    DirectoryItemDescription makeItemDesc(DirectoryEntry const& entry) const;

    /// Navigation

    void goUp();
    void goDown();
    void goBack();
    void goForward();
    bool navEnabled() const { return !viewData.renamingEntry; }
    void didNavigate() { viewData.renamingEntry = std::nullopt; }
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
    impl->delegate = delegate ? std::move(delegate) :
                                std::make_unique<DirectoryViewDelegate>();
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
        return !directoryLine.current().selection.empty();
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
    bool dispatched = false;
    for (auto [index, directory]: impl->directoryLine | enumerate) {
        if (!dispatched && directory.selectionModified) {
            impl->viewData.renamingEntry = std::nullopt;
            auto& sel = directory.selection;
            auto toOpen = [&] {
                if (sel.size() == 1 && sel.begin()->is_directory()) {
                    return sel.begin()->path();
                }
                return directory.path;
            }();
            impl->dispatch(DispatchToken::NextFrame, [=, impl = impl.get()] {
                impl->makeDirectoryCurrent(toOpen);
                impl->directoryLine.setCurrentIndex(index);
            });
            dispatched = true;
        }
        directory.selectionModified = false;
    }

    impl->viewData.updateLate();
    if (impl->showDebugger) {
        impl->displayDebugger();
    }
}

void DirectoryView::openDirectory(std::filesystem::path directory) {
    impl->directoryLine.setRoot(directory);
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

void Impl::displayBackground(ScannedDirectory& directory, size_t dirIndex) {
    ImGui::SetCursorPos({ ImGui::GetScrollX(), ImGui::GetScrollY() });
    float2 size =
        (float2)ImGui::GetCursorPos() + (float2)ImGui::GetContentRegionAvail();
    ImGui::SetNextItemAllowOverlap();
    bool active = ImGui::InvisibleButton("###background-button", size);
    if (active && viewData.hoveredEntry.empty()) {
        directory.selection.amend(std::vector<DirectoryEntry>{});
        directory.selectionModified = true;
    }
    itemDragTarget(DirectoryEntry(directory.path));
#if 0
    backgroundContextMenu(directory);
#endif
    if (ImGui::IsItemActive() && !viewData.isSelectionDragging &&
        viewData.hoveredEntry.empty())
    {
        beginDragSelection(dirIndex);
    }
    updateDragSelection(dirIndex);
}

void Impl::displayColumns() {
    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip;
    bool hasPreviewColumn = !directoryLine.back().selection.empty();
    int numColumns = (int)directoryLine.size() + hasPreviewColumn;

    if (!ImGui::BeginTable("column-view", numColumns, tableFlags)) {
        return;
    }
    // Setup
    float defaultWidth = 240;
    for (auto [index, directory]: directoryLine | enumerate) {
        ImGui::TableSetupColumn(directory.path.c_str(),
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    if (hasPreviewColumn) {
        ImGui::TableSetupColumn("##preview-column",
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    // Display
    for (auto&& [index, directory]: directoryLine | enumerate) {
        ImGui::TableNextColumn();
        displayColumn(directory, index);
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

void Impl::displayColumn(ScannedDirectory& directory, size_t lineIndex) {
    if (lineIndex == 0) {
        ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    auto clipRect = ImGui::GetCurrentWindowRead()->ClipRect;
    ImGui::BeginChild(directory.path.c_str(), contentSizeWithoutScrollbar());
    ImGui::PopStyleVar();
    ImGui::PushClipRect(clipRect.Min, clipRect.Max, false);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         ImGui::GetStyle().WindowPadding.y);
    for (size_t i = 0; i < directory.entries.size(); ++i) {
        displayColumnViewItem(directory, lineIndex, i);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         ImGui::GetStyle().WindowPadding.y);
    displayBackground(directory, lineIndex);
    ImGui::PopClipRect();
    ImGui::EndChild();
}

void Impl::displayColumnViewItem(ScannedDirectory& directory,
                                 size_t directoryIndex, size_t entryIndex) {
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
    auto& entry = directory.entries[entryIndex];
    bool selected = directory.selection.contains(entry);
    ImDrawFlags frameDrawFlags = 0;
    if (selected) {
        bool prevSelected =
            entryIndex > 0 &&
            directory.selection.contains(directory.entries[entryIndex - 1]);
        bool nextSelected =
            entryIndex < directory.entries.size() - 1 &&
            directory.selection.contains(directory.entries[entryIndex + 1]);
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
    displayEntry(directory, directoryIndex, entryIndex, itemBB, labelBB, iconBB,
                 TextAlign::Left, frameDrawFlags);
    ImGui::PopStyleVar(2);
    entryContextMenu(directory.entries[entryIndex], directory);
}

void Impl::displayPreviewColumn() {
    ImGui::BeginChild("preview-column", contentSizeWithoutScrollbar());
    auto& dir = directoryLine.current();
    if (dir.selection.empty()) {
        displayEmptyWithError("No entry selected");
    }
    else if (dir.selection.size() == 1) {
        if (!delegate->displayPreview(*dir.selection.begin())) {
            displayEmptyWithReason("No Preview");
        }
    }
    else {
        displayEmptyWithReason(utl::strcat(dir.selection.size(), " objects"));
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

void Impl::displayEntry(ScannedDirectory& directory, size_t directoryIndex,
                        size_t entryIndex, Rect itemBB, Rect labelBB,
                        Rect iconBB, TextAlign textAlign, int frameDrawFlags) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }
    auto& entry = directory.entries[entryIndex];
    auto itemDesc = makeItemDesc(entry);
    bool entrySelected = directory.selection.contains(entry);
    auto& g = *GImGui;
    auto& style = g.Style;
    if (viewData.isSelectionDragging &&
        directoryIndex == viewData.selectionRectDirectoryIndex)
    {
        viewData.entryBBs[entry.path()] = itemBB;
    }
    /// Button behaviour
    ItemButtonState buttonState = [&] {
        ImRect BB = ImRect(itemBB.lower_bound(), itemBB.upper_bound());
        ImGuiID ID = window->GetID(entry.path().filename().c_str());
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
        if (!entry.is_directory() && g.DragDropActive) {
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
                directory.selection.contains(entry))
            {
                selectionMode = SelectionMode::Add;
            }
        }
        if (wantSelectEntry) {
            buttonState.selected =
                directory.selection.amend(entry, selectionMode);
            directory.selectionModified = true;
            viewData.haveSelectedItem = true;
        }
        buttonState.activated =
            ImGui::GetMouseClickedCount(ImGuiMouseButton_Left) == 2;
        if (buttonState.activated) {
            activateEntry(entry);
        }
        buttonState.activatedByDragDrop =
            !g.IO.MouseReleased[ImGuiMouseButton_Left] &&
            !buttonState.selected && !buttonState.activated;
        return buttonState;
    }();
    if (buttonState.activated) {
        if (!entry.is_directory()) {
            delegate->openFile(entry);
        }
    }
    if (ImGui::IsItemHovered()) {
        viewData.hoveredEntry = entry.path();
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
    itemDragTarget(entry);
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
        displayEntryRenameField(entry, labelBB);
    }
    drawEntry(labelBB, iconBB, textAlign, itemDesc.label, itemDesc.iconName,
              textColor, isRenamingEntry);
}

static std::array<char, 512> RenameBuffer{};

void Impl::displayEntryRenameField(DirectoryEntry const& entry, Rect labelBB) {
    if (viewData.startRenaming) {
        std::string filename = entry.path().filename().string();
        if (filename.size() >= RenameBuffer.size()) {
            Logger::Error("Cannot rename file: name exceeds buffer size");
            viewData.renamingEntry = std::nullopt;
            return;
        }
        ImGui::SetKeyboardFocusHere();
        std::strncpy(RenameBuffer.data(), filename.c_str(),
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
        if (entry.is_directory()) {
            auto oldPath = entry.path();
            auto newPath = oldPath.parent_path() / RenameBuffer.data();
            entry.replace_filename(RenameBuffer.data());
            std::filesystem::rename(oldPath, newPath);
            if (auto* next = directoryLine.next();
                next && next->path == oldPath)
            {
                next->path = entry.path();
            }
            directoryLine.current().selection.transform(
                [&](DirectoryEntry elem) {
                if (elem.path() == oldPath) {
                    return entry;
                }
                return elem;
            });
        }
        else {
            delegate->renameEntry(entry, std::string(RenameBuffer.data()));
        }
        viewData.renamingEntry = std::nullopt;
        dispatch(DispatchToken::NextFrame, [this] { directoryLine.rescan(); });
    }
    else if (directoryLine.current().selection.size() == 1) {
        startRenaming(*directoryLine.current().selection.begin());
    }
}

void Impl::startRenaming(DirectoryEntry const& entry) {
    viewData.renamingEntry = entry;
    viewData.startRenaming = true;
}

void Impl::entryContextMenu(DirectoryEntry const& entry,
                            ScannedDirectory const& directory) {
    std::string popupID = utl::strcat("EntryPopup-", entry.path());
    ImGui::OpenPopupOnItemClick(popupID.c_str());
    if (ImGui::BeginPopup(popupID.c_str())) {
        if (!entry.is_directory() && ImGui::MenuItem("Open")) {
            activateSelectedEntries();
        }
        if (directory.selection.size() == 1 && ImGui::MenuItem("Rename")) {
            startRenaming(entry);
        }
        if (ImGui::MenuItem("Delete")) {
            Logger::Error("Deleting is not implemented");
        }
        ImGui::Separator();
        delegate->entryContextMenu(entry);
        ImGui::EndPopup();
    }
}

static std::vector<char> packDragDropPayload(
    std::span<uint8_t const> userData, SelectionManager const& selection) {
    std::vector<char> result(4);
    uint32_t userDataSize = (uint32_t)userData.size();
    std::memcpy(result.data(), &userDataSize, 4);
    result.insert(result.end(), userData.begin(), userData.end());
    for (auto& entry: selection) {
        auto str = entry.path().string();
        result.insert(result.end(), str.begin(), str.end());
        result.push_back('\0');
    }
    return result;
}

namespace {

struct DragDropDelivery {
    std::span<uint8_t const> userData;
    std::vector<std::filesystem::path> paths;
};

} // namespace

static DragDropDelivery unpackDragDropPayload(char const* data, size_t size,
                                              bool userDataOnly = false) {
    DragDropDelivery result;
    BL_ASSERT(size >= 4);
    uint32_t userDataSize = 0;
    std::memcpy(&userDataSize, data, 4);
    size_t i = 4 + userDataSize;
    result.userData =
        std::span((uint8_t const*)data + 4, (uint8_t const*)data + i);
    if (userDataOnly) {
        return result;
    }
    size_t j = i;
    for (; i < size; ++i) {
        if (data[i] != '\0') {
            continue;
        }
        result.paths.push_back(std::filesystem::path(data + j, data + i));
        j = i + 1;
    }
    return result;
}

std::span<uint8_t const> poppy::peekFileDragDropUserData() {
    auto* payload = ImGui::GetDragDropPayload();
    auto result = unpackDragDropPayload((char const*)payload->Data,
                                        payload->DataSize,
                                        /* userDataOnly: */ true);
    return result.userData;
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
            auto userData =
                delegate->makeDragDropUserData(directory.selection.elements());
            auto payload =
                packDragDropPayload(std::move(userData), directory.selection);
            ImGui::SetDragDropPayload(delegate->makeDragDropType().c_str(),
                                      payload.data(), payload.size(),
                                      ImGuiCond_Once);
        }
        if (auto* payload = ImGui::GetDragDropPayload();
            payload && payload->DataType == delegate->makeDragDropType())
        {
            float2 pos = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto delivery = unpackDragDropPayload((char const*)payload->Data,
                                                  payload->DataSize);
            for (auto [index, path]: delivery.paths | enumerate) {
                drawFn(DirectoryEntry(path), pos + index * float2(10, 10));
            }
        }
        ImGui::EndDragDropSource();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

static bool moveElement(std::filesystem::path const& element,
                        std::filesystem::path const& to) {
    BL_EXPECT(std::filesystem::is_directory(to));
    if (element.parent_path() == to) {
        return false;
    }
    try {
        std::filesystem::rename(element, to / element.filename());
        Logger::Trace("Moved entry to ", to);
        return true;
    }
    catch (std::filesystem::filesystem_error const& e) {
        Logger::Error("Failed to move file ", element, ": ", e.what());
        return false;
    }
}

void Impl::itemDragTarget(DirectoryEntry const& entry) {
    if (!entry.is_directory()) {
        return;
    }
    DragDropDelivery delivery;
    if (auto* payload = ImGui::GetDragDropPayload();
        payload && payload->DataType == delegate->makeDragDropType().c_str())
    {
        delivery = unpackDragDropPayload((char const*)payload->Data,
                                         payload->DataSize);
        if (delivery.paths.empty()) {
            return;
        }
        /// Not drop into ourself
        if (ranges::contains(delivery.paths, entry.path())) {
            return;
        }
        /// Not drop into parent directory
        if (delivery.paths.front().parent_path() == entry) {
            return;
        }
    }
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }
    ImGuiPayload const* payload = ImGui::AcceptDragDropPayload(
        std::string(delegate->makeDragDropType()).c_str());
    if (!payload) {
        ImGui::EndDragDropTarget();
        return;
    }
    for (auto& path: delivery.paths) {
        if (!path.has_parent_path() || path.parent_path() == entry.path()) {
            continue;
        }
        dispatch(DispatchToken::NextFrame, [=, this] {
            if (moveElement(path, entry.path())) {
                directoryLine.rescan();
            }
        });
    }
    ImGui::EndDragDropTarget();
}

void Impl::beginDragSelection(size_t directoryIndex) {
    viewData.isSelectionDragging = true;
    viewData.selectionRectBegin = (float2)ImGui::GetMousePos();
    viewData.selectionRectDirectoryIndex = directoryIndex;
    directoryLine[directoryIndex].selection.beginCandidates(getSelectionMode());
}

void Impl::updateDragSelection(size_t directoryIndex) {
    if (!viewData.isSelectionDragging ||
        viewData.selectionRectDirectoryIndex != directoryIndex)
    {
        return;
    }
    auto& g = *GImGui;
    auto& window = *g.CurrentWindow;
    float2 pos1 = viewData.selectionRectBegin;
    float2 pos2 = ImGui::GetMousePos();
    Rect rect(pos1, pos2);
    auto& dir = directoryLine[directoryIndex];
    SelectionManager::SetType candidates;
    for (auto& entry: dir.entries) {
        if (vml::do_intersect(rect, viewData.entryBBs[entry.path()])) {
            candidates.insert(entry);
        }
    }
    dir.selection.setCandidates(candidates);
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        dir.selection.applyCandidates();
        dir.selectionModified = true;
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

void Impl::activateEntry(DirectoryEntry const& entry) {
    if (!entry.is_directory()) {
        delegate->openFile(entry);
    }
}

void Impl::activateSelectedEntries() {
    for (auto& entry: directoryLine.current().selection) {
        activateEntry(entry);
    }
}

DirectoryItemDescription Impl::makeItemDesc(DirectoryEntry const& entry) const {
    std::string iconName = [&]() -> std::string {
        if (!entry.is_directory()) {
            return delegate->selectIcon(entry);
        }
        return "folder-open-empty";
    }();
    return { .label = entry.path().filename(),
             .iconName = std::move(iconName) };
}

void Impl::goUp() {
    if (navEnabled() && directoryLine.goUp()) {
        history.push(directoryLine.current().path);
        didNavigate();
    }
}

void Impl::goDown() {
    if (navEnabled() && directoryLine.goDown()) {
        history.push(directoryLine.current().path);
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
    if (directoryLine.current().path == path) {
        return;
    }
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
    auto& dir = directoryLine.current();
    BL_ASSERT(mode != SelectionMode::Add);
    if (dir.selection.empty()) {
        return;
    }
    auto last = dir.selection.last();
    BL_ASSERT(last);
    auto itr = ranges::find(dir.entries, *last);
    BL_ASSERT(itr != dir.entries.end());
    auto index = itr - dir.entries.begin() + offset;
    if (index >= 0 && index < (ssize_t)dir.entries.size()) {
        dir.selection.amend(dir.entries[index], mode);
        dir.selectionModified = true;
    }
}

void Impl::createFolder() {
    auto& dir = directoryLine.current();
    auto baseName = "New Folder";
    std::string name = baseName;
    int i = 2;
    while (true) {
        if (!dir.find(name)) {
            break;
        }
        name = utl::strcat(baseName, " ", i++);
    }
    std::filesystem::create_directory(dir.path / name);
    directoryLine.rescan();
    auto entry = dir.find(name);
    if (!entry) {
        Logger::Error(
            "Internal error occurred. Failed to locate newly created folder");
        return;
    }
    startRenaming(*entry);
}

void Impl::displayDebugger() {
    ImGui::Begin("DirViewDebugger");
    ImGui::Text("Current directory: %s", directoryLine.current().path.c_str());
    ImGui::Text("Hovered entry: %s", viewData.hoveredEntry.c_str());
    if (viewData.isSelectionDragging) {
        ImGui::SeparatorText("Selection dragging");
        ImGui::Text("selectionRectBegin: %f, %f",
                    (double)viewData.selectionRectBegin.x,
                    (double)viewData.selectionRectBegin.y);
        ImGui::Text("selectionRectDirectoryIndex: %d",
                    (int)viewData.selectionRectDirectoryIndex);
    }
    ImGui::Text("Rename Buffer: %s", RenameBuffer.data());
    if (viewData.renamingEntry) {
        ImGui::Text("renamingEntry: %s",
                    viewData.renamingEntry->path().c_str());
    }
    ImGui::SeparatorText("Directory Line");
    for (auto& dir: directoryLine) {
        if (ImGui::TreeNodeEx(dir.path.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::TreeNode("Entries")) {
                for (auto& entry: dir.entries) {
                    ImGui::Text("%s", entry.path().c_str());
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Selection", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (auto& entry: dir.selection) {
                    ImGui::Text("%s", entry.path().c_str());
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

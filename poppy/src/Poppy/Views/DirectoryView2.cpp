#include "Poppy/Views/DirectoryView2.h"

#include <numeric>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>
#include <utl/filesystem_ext.hpp>
#include <utl/hashset.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Core/Debug.h"
#include "Poppy/Core/SelectionManager.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/Toolbar.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/FilesystemHistory_Merge.hpp"

using namespace bloom;
using namespace poppy;
using namespace vml::short_types;
using namespace ranges::views;

namespace {

struct DirViewTest: View {
    DirViewTest() {}
    void init() override {
        impl.init(editor().makeEmitter());
        impl.openDirectory("/Users/indigo/dev/bloom/projects");
        setToolbar(impl.makeToolbar());
        setPadding({ 0, 0 });
    }
    void display() override { impl.display(); }
    DirectoryView2 impl;
};

} // namespace

POPPY_REGISTER_VIEW(DirViewTest, "DirViewTest");

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

using Rect = vml::rectangle<float>;

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

struct ScannedDirectory {
    static ScannedDirectory Build(std::filesystem::path path) {
        ScannedDirectory result;
        result.path = path;
        result.rescan();
        return result;
    }

    void rescan() {
        entries.clear();
        for (auto entry: std::filesystem::directory_iterator(path)) {
            entries.push_back(entry);
        }
    }

    std::filesystem::path path;
    std::vector<std::filesystem::directory_entry> entries;
    BasicSelectionManager<std::filesystem::path> selection;

private:
    ScannedDirectory() = default;
};

struct DirectoryLine {
    void setRoot(std::filesystem::path const& directory) {
        _root = directory;
        line.clear();
        line.push_back(ScannedDirectory::Build(directory));
    }

    bool open(std::filesystem::path leafDirectory) {
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
                line.push_back(ScannedDirectory::Build(path));
            }
        }
        for (auto [line, next]: zip(line, line | drop(1))) {
            line.selection.set(next.path);
        }
        return true;
    }

    void rescan() {
        for (auto& directory: line) {
            directory.rescan();
        }
    }

    bool canGoUp() const { return line.size() > 1; }

    bool goUp() {
        if (!canGoUp()) {
            Logger::Error("Cannot go up, already in root directory");
            return false;
        }
        line.pop_back();
        return true;
    }

    ScannedDirectory& current() { return back(); }
    ScannedDirectory const& current() const { return back(); }
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

private:
    std::filesystem::path _root;
    std::vector<ScannedDirectory> line;
};

enum class TextAlign { Left, Right, Center };

} // namespace

struct ActiveViewData {
    bool haveSelectedItem = false;
};

struct ViewData: ActiveViewData {
    void resetActiveData() { static_cast<ActiveViewData&>(*this) = {}; }
};

using Impl = DirectoryView2::Impl;

struct DirectoryView2::Impl: Emitter {
    Toolbar makeToolbar();

    void displayColumns();
    void displayColumn(ScannedDirectory& directory, size_t lineIndex);
    void displayColumnViewItem(ScannedDirectory& directory,
                               size_t directoryIndex, size_t entryIndex);
    void displayPreviewColumn();

    void displayEntry(ScannedDirectory& directory, size_t directoryIndex,
                      size_t entryIndex, Rect itemBB, Rect labelBB, Rect iconBB,
                      TextAlign textAlign, int frameDrawFlags);
    void itemDragSource(ScannedDirectory const& directory,
                        utl::function_view<void(
                            std::filesystem::directory_entry const&, float2)>
                            drawFn);
    void itemDragTarget(ScannedDirectory const& directory, size_t entryIndex);

    DirectoryItemDescription makeItemDesc(
        std::filesystem::directory_entry const& entry) const;

    /// Navigation

    void goUp();
    void goBack();
    void goForward();
    bool canGoUp() const;
    bool canGoBack() const;
    bool canGoForward() const;
    void openDirectory(std::filesystem::path const& path);

    void displayDebugger();

    Layout layout = Layout::Columns;
    DirectoryLine directoryLine;
    FilesystemHistory history;
    ViewData viewData;
    size_t activeDirectoryIndex = 0;
};

DirectoryView2::DirectoryView2(): impl(std::make_unique<Impl>()) {}

void DirectoryView2::init(Emitter emitter) {
    impl->assignEmitter(std::move(emitter));
}

Toolbar DirectoryView2::makeToolbar() { return impl->makeToolbar(); }

void DirectoryView2::display() {
    switch (impl->layout) {
    case Layout::Columns:
        impl->displayColumns();
        break;
    default:
        BL_UNIMPLEMENTED();
        break;
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
        ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        impl->viewData.resetActiveData();
    }
    impl->displayDebugger();
}

void DirectoryView2::openDirectory(std::filesystem::path directory) {
    impl->directoryLine.setRoot(directory);
    impl->history.push(directory);
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
        CustomToolbarElement([this](float2 size) {
//            addressbar(size);
        }),
        ToolbarDropdownMenu()
            .content([this]{
//                enumCombo(desc.layout);
            })
            .tooltip("Layout"),
        ToolbarIconButton("cw")
            .onClick([this]{
//                rescan();
            })
            .tooltip("Rescan")
    }; // clang-format on
}

void Impl::displayColumns() {
    ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable |
                                 ImGuiTableFlags_ScrollX |
                                 ImGuiTableFlags_SizingFixedFit;
    int numColumns = (int)directoryLine.size() + 1;
    if (!ImGui::BeginTable("column-view", numColumns, tableFlags,
                           (float2)ImGui::GetContentRegionAvail()))
    {
        return;
    }
    // Setup
    float defaultWidth = 240;
    for (auto [index, directory]: directoryLine | enumerate) {
        ImGui::TableSetupColumn(directory.path.c_str(),
                                ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    }
    ImGui::TableSetupColumn("##preview-column",
                            ImGuiTableColumnFlags_WidthFixed, defaultWidth);
    // Display
    for (auto&& [index, directory]: directoryLine | enumerate) {
        ImGui::TableNextColumn();
        displayColumn(directory, index);
    }
    ImGui::TableNextColumn();
    displayPreviewColumn();
    ImGui::EndTable();
}

void Impl::displayColumn(ScannedDirectory& directory, size_t lineIndex) {
    if (lineIndex == 0) {
        ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::BeginChild(directory.path.c_str());
    ImGui::PopStyleVar();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() +
                         ImGui::GetStyle().WindowPadding.y);
    for (size_t i = 0; i < directory.entries.size(); ++i) {
        displayColumnViewItem(directory, lineIndex, i);
    }
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
    bool selected = directory.selection.contains(entry.path());
    ImDrawFlags frameDrawFlags = 0;
    if (selected) {
        bool prevSelected = entryIndex > 0 &&
                            directory.selection.contains(
                                directory.entries[entryIndex - 1].path());
        bool nextSelected = entryIndex < directory.entries.size() - 1 &&
                            directory.selection.contains(
                                directory.entries[entryIndex + 1].path());
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
    //    handleItemButtonState(buttonState, entry);
    //    if (buttonState.selected &&
    //    ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    //        gotoNextFrame(entry->is_directory() ? entry->path() :
    //        entry->path().parent_path());
    //    }
}

void Impl::displayPreviewColumn() {
    ImGui::BeginChild("preview-column",
                      { 0, -2 /* to prevent mini scroll y */ });
    displayEmptyWithReason("No Preview");
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
    bool entrySelected = directory.selection.contains(entry.path());
    auto& g = *GImGui;
    auto& style = g.Style;
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
                           ImGuiButtonFlags_PressedOnRelease;
        bool buttonActive = ImGui::ButtonBehavior(BB, ID, &buttonState.hovered,
                                                  &buttonState.held,
                                                  buttonFlags);
        if (!entry.is_directory() && g.DragDropActive) {
            buttonState.hovered = false;
        }
        if (!buttonActive) {
            return buttonState;
        }
        if (!entrySelected) {
            if (g.IO.MouseClicked[ImGuiMouseButton_Left]) {
                buttonState.selected = directory.selection.amend(entry.path());
                viewData.haveSelectedItem = true;
            }
        }
        else if (!viewData.haveSelectedItem) {
            if (g.IO.MouseReleased[ImGuiMouseButton_Left]) {
                buttonState.selected = directory.selection.amend(entry.path());
            }
        }
        buttonState.activated = g.IO.MouseClickedCount[ImGuiMouseButton_Left] ==
                                2;
        buttonState.activatedByDragDrop =
            !g.IO.MouseReleased[ImGuiMouseButton_Left] &&
            !buttonState.selected && !buttonState.activated;
        return buttonState;
    }();
    itemDragSource(directory,
                   [&](std::filesystem::directory_entry const& draggingItem,
                       float2 position) {
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
    itemDragTarget(directory, entryIndex);
    // Draw Frame
    auto [frameColor, textColor] = [&]() -> std::pair<ImU32, ImU32> {
        if (!entrySelected) {
            return { 0, ImGui::GetColorU32(ImGuiCol_Text) };
        }
        if (directoryIndex == activeDirectoryIndex) {
            return { ImGui::GetColorU32(ImGuiCol_Header), IM_COL32_WHITE };
        }
        return { ImGui::ColorConvertFloat4ToU32(
                     getStyleColors().itemInactiveSelected),
                 ImGui::GetColorU32(ImGuiCol_Text) };
    }();
    if (entrySelected) {
        drawEntryFrame(itemBB, frameColor, frameDrawFlags);
    }
#if 0
    if (entry->renaming) {
        itemRenameField(entry.index(), entry,
                        (ImVec2)labelBB.lower_bound() + window->Scroll - window->Pos,
                        itemBB.size().x);
    }
#endif
    drawEntry(labelBB, iconBB, textAlign, itemDesc.label, itemDesc.iconName,
              textColor,
              /* renaming: */ false);
#if 0
    itemPopup(entry, itemDesc);
#endif
    if (buttonState.selected) {
        dispatch(DispatchToken::NextFrame, [=, this] {
            openDirectory(entry.is_directory() ? entry.path() :
                                                 entry.path().parent_path());
            activeDirectoryIndex = directoryIndex;
        });
    }
}

static std::vector<char> packDragDropPayload(
    BasicSelectionManager<std::filesystem::path> const& selection) {
    std::vector<char> result;
    for (auto& path: selection.elements()) {
        auto str = path.string();
        result.insert(result.end(), str.begin(), str.end());
        result.push_back('\0');
    }
    return result;
}

static std::vector<std::filesystem::path> unpackDragDropPayload(
    char const* data, size_t size) {
    std::vector<std::filesystem::path> result;
    size_t j = 0;
    for (size_t i = 0; i < size; ++i) {
        if (data[i] == '\0') {
            result.push_back(std::filesystem::path(data + j, data + i));
            j = i + 1;
        }
    }
    return result;
}

void Impl::itemDragSource(
    ScannedDirectory const& directory,
    utl::function_view<void(std::filesystem::directory_entry const&, float2)>
        drawFn) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, 0x0);
    if (ImGui::BeginDragDropSource()) {
        bool havePayload = ImGui::GetDragDropPayload() &&
                           ImGui::GetDragDropPayload()->Data;
        if (!havePayload) {
            auto payload = packDragDropPayload(directory.selection);
            ImGui::SetDragDropPayload("FILE_DRAG_DROP", payload.data(),
                                      payload.size(), ImGuiCond_Once);
        }
        if (auto* payload = ImGui::GetDragDropPayload();
            payload && payload->DataType == std::string_view("FILE_DRAG_DROP"))
        {
            float2 pos = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto paths = unpackDragDropPayload((char const*)payload->Data,
                                               payload->DataSize);
            for (auto [index, path]: paths | enumerate) {
                drawFn(std::filesystem::directory_entry(path),
                       pos + index * float2(10, 10));
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

void Impl::itemDragTarget(ScannedDirectory const& directory,
                          size_t entryIndex) {
    auto& entry = directory.entries[entryIndex];
    if (!entry.is_directory()) {
        return;
    }
    std::vector<std::filesystem::path> paths;
    if (auto* payload = ImGui::GetDragDropPayload();
        payload && payload->DataType == std::string_view("FILE_DRAG_DROP"))
    {
        paths = unpackDragDropPayload((char const*)payload->Data,
                                      payload->DataSize);
        if (ranges::contains(paths, entry.path())) {
            return;
        }
    }
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }
    ImGuiPayload const* payload =
        ImGui::AcceptDragDropPayload("FILE_DRAG_DROP");
    if (!payload) {
        ImGui::EndDragDropTarget();
        return;
    }
    for (auto& path: paths) {
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

DirectoryItemDescription Impl::makeItemDesc(
    std::filesystem::directory_entry const& entry) const {
    std::string iconName = [&]() -> std::string {
        if (!entry.is_directory()) {
            return "doc";
        }
        return "folder-open-empty";
    }();
    return { .label = entry.path().filename(),
             .iconName = std::move(iconName) };
}

void Impl::goUp() {
    if (directoryLine.goUp()) {
        history.push(directoryLine.current().path);
    }
}

void Impl::goBack() {
    history.pop();
    directoryLine.open(history.current());
}

void Impl::goForward() {
    history.unpop();
    directoryLine.open(history.current());
}

bool Impl::canGoUp() const { return directoryLine.canGoUp(); }

bool Impl::canGoBack() const { return history.canPop(); }

bool Impl::canGoForward() const { return history.canUnpop(); }

void Impl::openDirectory(std::filesystem::path const& path) {
    if (directoryLine.current().path == path) {
        return;
    }
    if (directoryLine.open(path)) {
        history.push(path);
    }
}

void Impl::displayDebugger() {
    ImGui::Begin("DirViewDebugger");
    ImGui::Text("Active Directory: %s",
                directoryLine[activeDirectoryIndex].path.c_str());
    ImGui::SeparatorText("Directory Line");
    for (auto& dir: directoryLine) {
        if (ImGui::TreeNodeEx(dir.path.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SeparatorText("Entries");
            for (auto& entry: dir.entries) {
                ImGui::Text("%s", entry.path().c_str());
            }
            ImGui::SeparatorText("Selection");
            for (auto& path: dir.selection.elements()) {
                ImGui::Text("%s", path.c_str());
            }
            ImGui::TreePop();
        }
    }
    ImGui::SeparatorText("History");
    history.displayDebugger();
    ImGui::End();
}

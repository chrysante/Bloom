#include "Poppy/UI/Dockspace.h"

#include <set>
#include <unordered_set>

#include <imgui.h>
#include <imgui_internal.h>
#include <mtl/mtl.hpp>
#include <utl/function_view.hpp>
#include <utl/scope_guard.hpp>
#include <utl/stack.hpp>

#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace poppy;
using namespace bloom;
using namespace mtl::short_types;

static constexpr auto MainWindowID = "__MainWindow__";
static constexpr auto MainDockspaceID = "__MainWindow_Dockspace__";

template <typename T>
static void makeUnique(utl::vector<T>& v) {
    auto last = std::unique(v.begin(), v.end());
    v.resize(last - v.begin());
}

static void withWindowSizeConstraints(mtl::float2 minSize, auto&& block) {
    ImGuiStyle& style = ImGui::GetStyle();
    utl::scope_guard restore = [&, oldSize = style.WindowMinSize] {
        style.WindowMinSize = oldSize;
    };
    style.WindowMinSize = minSize;
    block();
}

Dockspace::Dockspace() = default;

void Dockspace::display(Window& window) {
    mainWindow();
    setToolbarHeight(window.toolbarHeight());
    displayToolbar(window);
}

void Dockspace::setLeftToolbar(Toolbar tb) {
    tb.setHeight(45);
    toolbars[0] = std::move(tb);
}

void Dockspace::setCenterToolbar(Toolbar tb) {
    tb.setHeight(45);
    toolbars[1] = std::move(tb);
}

void Dockspace::setRightToolbar(Toolbar tb) {
    tb.setHeight(45);
    toolbars[2] = std::move(tb);
}

void Dockspace::setToolbarHeight(float height) {
    for (auto& toolbar: toolbars) {
        toolbar.setHeight(height);
    }
}

void Dockspace::setInsets(float left, float right) {
    leftInset = left;
    rightInset = right;
}

void Dockspace::mainWindow() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    auto windowPosition =
        float2(viewport->Pos) + float2(0, toolbars[0].getHeight());
    auto windowSize =
        float2(viewport->Size) - float2(0, toolbars[0].getHeight());
    ImGui::SetNextWindowPos(windowPosition);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags windowFlags = 0;
    windowFlags |= ImGuiWindowFlags_NoTitleBar;
    windowFlags |= ImGuiWindowFlags_NoResize;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    windowFlags |= ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoNavFocus;
    auto& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        float2(1, 1) * (float2)style.FramePadding);
    withFont(Font::UIDefault(), [&] {
        ImGui::Begin(MainWindowID, nullptr, windowFlags);
        withWindowSizeConstraints(minWindowSize,
                                  [&] { submitMasterDockspace(); });
        ImGui::End();
    });
    ImGui::PopStyleVar(6);
}

void Dockspace::submitMasterDockspace() {
    ImGuiDockNodeFlags flags = 0;
    flags |= ImGuiDockNodeFlags_NoWindowMenuButton;
    flags |= ImGuiDockNodeFlags_NoCloseButton;
    mainDockID = ImGui::GetID(MainDockspaceID);
    ImGui::DockSpace(mainDockID, /* size = */ {}, flags);
}

utl::small_vector<int, 2> Dockspace::getToolbarSpacing() const {
    utl::vector<mtl::rectangle<int>> const potentialRects = [this] {
        ImGuiDockNode const* mainNode = ImGui::DockBuilderGetNode(mainDockID);
        BL_ASSERT(mainNode);
        utl::stack<ImGuiDockNode const*> stack;
        stack.push(mainNode);
        utl::vector<mtl::rectangle<int>> result;
        while (!stack.empty()) {
            auto* node = stack.pop();
            if ((int)node->Pos.y == (int)mainNode->Pos.y && node->IsVisible &&
                node != mainNode && (int2)node->Size != (int2)mainNode->Size)
            {
                result.push_back({ node->Pos, node->Size });
            }
            if (!node->IsLeafNode()) {
                stack.push(node->ChildNodes[0]);
                stack.push(node->ChildNodes[1]);
            }
        }
        return result;
    }();
    return [&potentialRects]() -> utl::small_vector<int, 2> {
        utl::vector<int> positions;
        for (auto rect: potentialRects) {
            positions.push_back(rect.lower_bound().x);
            positions.push_back(rect.upper_bound().x);
        }
        std::sort(positions.begin(), positions.end());
        makeUnique(positions);
        positions.erase(positions.begin());
        if (!positions.empty()) {
            positions.erase(positions.end() - 1);
        }
        for (int i = 0; auto& p: positions) {
            if (i++ % 2 == 0) {
                p += 1;
            }
            else {
                p -= 1;
            }
        }
        makeUnique(positions);
        switch (positions.size()) {
        case 0:
            return {};
        case 1:
            return { positions.front() };
        default:
            return { positions.front(), positions.back() };
        }
    }();
}

[[nodiscard]] static bool beginToolbarWindow(char const* ID, float2 size) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiWindow* mainWindow = ImGui::FindWindowByName(MainWindowID);
    float2 position = { 0, viewport->Pos.y + mainWindow->MenuBarHeight() };
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoDocking;
    flags |= ImGuiWindowFlags_NoTitleBar;
    flags |= ImGuiWindowFlags_NoResize;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoNavFocus;
    flags |= ImGuiWindowFlags_NoFocusOnAppearing;
    flags |= ImGuiWindowFlags_NoScrollbar;
    flags |= ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    bool result = ImGui::Begin(ID, NULL, flags);
    ImGui::PopStyleVar(2);
    return result;
}

static void toolbarBGWindow(float2 size) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool open = beginToolbarWindow("Main_Toolbar_BG", size);
    ImGui::PopStyleVar();
    if (!open) {
        return;
    }
    auto* window = ImGui::GetCurrentWindow();
    auto* DL = window->DrawList;
    // Background
    auto bgCol = ImGui::ColorConvertFloat4ToU32(
        ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
    DL->AddRectFilled(window->Pos, window->Size, bgCol);
    // Bottom separator
    auto separatorCol = ImGui::ColorConvertFloat4ToU32(
        GImGui->Style.Colors[ImGuiCol_Separator]);
    DL->AddLine(float2(0, window->Pos.y + window->Size.y - 1),
                float2(window->Size.x, window->Pos.y + window->Size.y - 1),
                separatorCol);
    ImGui::End();
}

static void toolbarWindow(float2 size, utl::function_view<void()> content) {
    toolbarBGWindow(size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    bool open = beginToolbarWindow("Main_Toolbar", size);
    ImGui::PopStyleColor();
    if (!open) {
        return;
    }
    content();
    if (ImGui::IsWindowFocused() && GImGui->WindowsFocusOrder.size() > 1 &&
        ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left])
    {
        ImGui::FocusWindow(*(GImGui->WindowsFocusOrder.end() - 2));
    }
    ImGui::End();
}

/// Simulates window titlebar behaviour on the toolbar background
static void titlebarBehaviour(Window& window, ImGuiViewport* viewport,
                              float toolbarHeight) {
    ImGui::BeginDisabled();
    float2 windowPadding = GImGui->Style.WindowPadding;
    ImGui::SetCursorPos(-windowPadding);
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("Main_Toolbar_Background_Button",
                           { viewport->Size.x, toolbarHeight });
    ImGui::EndDisabled();
    if (GImGui->LastItemData.ID == GImGui->HoveredIdPreviousFrame) {
        if (GImGui->IO.MouseClickedCount[ImGuiMouseButton_Left] == 2) {
            window.zoom();
        }
        window.setMovable(true);
    }
    else {
        window.setMovable(false);
    }
}

void Dockspace::displayToolbar(Window& window) {
    float height = toolbars[0].getHeight();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    auto spacing = getToolbarSpacing();
    float width = viewport->Size.x;
    switch (spacing.size()) {
    case 0:
        break;
    case 1:
        // In central third
        if (spacing[0] >= width * 0.33 && spacing[0] <= width * 0.67) {
            spacing.clear();
        }
        break;
    case 2:
        break;
    default:
        BL_UNREACHABLE();
    }
    for (auto itr = spacing.begin(); itr != spacing.end();) {
        if (*itr >= viewport->Size.x * 0.33 && *itr <= viewport->Size.x * 0.67)
        {
            itr = spacing.erase(itr);
        }
        else {
            ++itr;
        }
    }
    BL_ASSERT(spacing.size() <= 2);
    toolbarWindow({ viewport->Size.x, height }, [&] {
        titlebarBehaviour(window, viewport, height);
        switch (spacing.size()) {
        case 0:
            toolbarLayoutOne(spacing);
            break;
        case 1:
            toolbarLayoutTwo(spacing);
            break;
        case 2:
            toolbarLayoutThree(spacing);
            break;
        default:
            BL_UNREACHABLE();
        }
    });
}

void Dockspace::toolbarLayoutOne(utl::small_vector<int, 2> /* spacing */) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float2 padding = GImGui->Style.WindowPadding;
    float t0Width = toolbars[0].getWidthWithoutSpacers();
    float t2Width = toolbars[2].getWidthWithoutSpacers();
    ImGui::SetCursorPos(padding);
    toolbars[0].display(t0Width);
    ImGui::SetCursorPos(ImVec2(padding.x + t0Width + padding.x, padding.y));
    toolbars[1].display(viewport->Size.x - t0Width - t2Width - 4 * padding.x);
    ImGui::SetCursorPos(
        ImVec2(viewport->Size.x - t2Width - padding.x, padding.y));
    toolbars[2].display(t2Width);
}

void Dockspace::toolbarLayoutTwo(utl::small_vector<int, 2> spacing) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float2 padding = GImGui->Style.WindowPadding;
    float spacer = spacing[0];
    float position[2] = { 0, spacer };
    float width[2] = { spacer, viewport->Size.x - spacer };
    if (spacer >= viewport->Size.x / 2) {
        // Left section is big
        // 1. section
        float t0Width = toolbars[0].getWidthWithoutSpacers();
        ImGui::SetCursorPos(ImVec2(position[0] + padding.x, padding.y));
        toolbars[0].display(t0Width);
        ImGui::SetCursorPos(
            ImVec2(position[0] + padding.x + t0Width + padding.x, padding.y));
        toolbars[1].display(width[0] - t0Width - 3 * padding.x);
        // 2. section
        ImGui::SetCursorPos(ImVec2(position[1] + padding.x, padding.y));
        toolbars[2].display(width[1] - 2 * padding.x);
    }
    else {
        // Right section is big
        // 1. section
        ImGui::SetCursorPos(ImVec2(position[0] + padding.x, padding.y));
        toolbars[0].display(width[0] - 2 * padding.x);
        // 2. section
        float const t2Width = toolbars[2].getWidthWithoutSpacers();
        ImGui::SetCursorPos(ImVec2(position[1] + padding.x, padding.y));
        toolbars[1].display(width[1] - t2Width - 3 * padding.x);
        ImGui::SetCursorPos(
            ImVec2(viewport->Size.x - padding.x - t2Width, padding.y));
        toolbars[2].display(t2Width);
    }
}

void Dockspace::toolbarLayoutThree(utl::small_vector<int, 2> spacing) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float2 padding = GImGui->Style.WindowPadding;
    float position[3] = { 0, (float)spacing[0], (float)spacing[1] };
    float width[3] = { (float)spacing[0], (float)(spacing[1] - spacing[0]),
                       viewport->Size.x - spacing[1] };
    for (std::size_t i = 0; i < 3; ++i) {
        ImGui::SetCursorPos(ImVec2(position[i] + padding.x, padding.y));
        toolbars[i].display(width[i] - 2 * padding.x);
    }
}

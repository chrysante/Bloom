#include "Poppy/UI/Dockspace.h"

#include <set>
#include <unordered_set>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <utl/function_view.hpp>
#include <utl/scope_guard.hpp>
#include <utl/stack.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/ImGuiHelpers.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

static constexpr auto MainWindowID = "__MainWindow__";
static constexpr auto MainDockspaceID = "__MainWindow_Dockspace__";

static void makeUnique(auto& vector) {
    auto last = std::unique(vector.begin(), vector.end());
    vector.resize(last - vector.begin());
}

static void eraseIf(auto& vector, auto pred) {
    auto itr = ranges::remove_if(vector, pred);
    vector.erase(itr, vector.end());
}

static void withWindowSizeConstraints(vml::float2 minSize, auto&& block) {
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

ToolbarStyle const tbStyle{ .padding = float2(0, 0),
                            .flags = ToolbarFlags::NoDecoration };

void Dockspace::setLeftToolbar(Toolbar tb) {
    tb.setStyle(tbStyle);
    toolbars[0] = std::move(tb);
}

void Dockspace::setCenterToolbar(Toolbar tb) {
    tb.setStyle(tbStyle);
    toolbars[1] = std::move(tb);
}

void Dockspace::setRightToolbar(Toolbar tb) {
    tb.setStyle(tbStyle);
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
    withFont(FontDesc::UIDefault(), [&] {
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

static bool usingViewports() {
    return ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable;
}

static float2 getPosition(Window const& window) {
    return usingViewports() ? (float2)window.position() : float2();
}

[[nodiscard]] static bool beginToolbarWindow(char const* ID, float2 position,
                                             float2 size) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
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

static void toolbarBGWindow(float2 position, float2 size) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool open = beginToolbarWindow("Main_Toolbar_BG", position, size);
    ImGui::PopStyleVar();
    if (!open) {
        return;
    }
    auto* imguiWindow = ImGui::GetCurrentWindow();
    auto* DL = imguiWindow->DrawList;
    // Background
    auto bgCol = ImGui::ColorConvertFloat4ToU32(
        ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
    DL->AddRectFilled(position, position + (float2)imguiWindow->Size, bgCol);
    // Bottom separator
    auto separatorCol = ImGui::ColorConvertFloat4ToU32(
        GImGui->Style.Colors[ImGuiCol_Separator]);
    float2 linePos = position + float2(0, imguiWindow->Size.y - 1);
    DL->AddLine(linePos, linePos + float2(imguiWindow->Size.x, 0),
                separatorCol);
    ImGui::End();
}

static void toolbarWindow(float2 position, float2 size,
                          utl::function_view<void()> content) {
    toolbarBGWindow(position, size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    bool open = beginToolbarWindow("Main_Toolbar", position, size);
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
static void titlebarBehaviour(Window& window, float toolbarHeight) {
    if (toolbarHeight <= 0) {
        return;
    }
    ImGui::BeginDisabled();
    float width = window.size().x;
    float2 windowPadding = GImGui->Style.WindowPadding;
    ImGui::SetCursorPos(-windowPadding);
    ImGui::SetNextItemAllowOverlap();
    ImGui::InvisibleButton("Main_Toolbar_Background_Button",
                           { width, toolbarHeight });
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

static utl::small_vector<int, 2> getToolbarSpacing(Window& window,
                                                   unsigned mainDockID) {
    auto potentialRects = [&] {
        int2 windowPos = getPosition(window);
        ImGuiDockNode const* mainNode = ImGui::DockBuilderGetNode(mainDockID);
        BL_ASSERT(mainNode);
        utl::stack<ImGuiDockNode const*> stack;
        stack.push(mainNode);
        std::vector<vml::rectangle<int>> result;
        while (!stack.empty()) {
            auto* node = stack.pop();
            if ((int)node->Pos.y == (int)mainNode->Pos.y && node->IsVisible &&
                node != mainNode && (int2)node->Size != (int2)mainNode->Size)
            {
                result.push_back({ (int2)node->Pos - windowPos, node->Size });
            }
            if (!node->IsLeafNode()) {
                stack.push(node->ChildNodes[0]);
                stack.push(node->ChildNodes[1]);
            }
        }
        return result;
    }();
    return [&]() -> utl::small_vector<int, 2> {
        std::vector<int> positions;
        for (auto rect: potentialRects) {
            positions.push_back(rect.lower_bound().x);
            positions.push_back(rect.upper_bound().x);
        }
        ranges::sort(positions);
        makeUnique(positions);
        eraseIf(positions,
                [&](int p) { return p <= 0 || p >= window.size().x; });
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

void Dockspace::displayToolbar(Window& window) {
    float height = toolbars[0].getHeight();
    auto spacing = getToolbarSpacing(window, mainDockID);
    float width = window.size().x;
    eraseIf(spacing, [&](int pos) {
        return pos >= width * 0.33 && pos <= width * 0.67;
    });
    BL_ASSERT(spacing.size() <= 2);
    toolbarWindow(getPosition(window), { width, height }, [&] {
        titlebarBehaviour(window, height);
        switch (spacing.size()) {
        case 0:
            toolbarLayoutOne(window, spacing);
            break;
        case 1:
            toolbarLayoutTwo(window, spacing);
            break;
        case 2:
            toolbarLayoutThree(window, spacing);
            break;
        default:
            BL_UNREACHABLE();
        }
    });
}

void Dockspace::toolbarLayoutOne(Window& window,
                                 std::span<int const> /* spacing */) {
    float windowWidth = window.size().x;
    float2 padding = GImGui->Style.WindowPadding;
    float t0Width = toolbars[0].getWidthWithoutSpacers();
    float t2Width = toolbars[2].getWidthWithoutSpacers();
    ImGui::SetCursorPos(padding);
    toolbars[0].display(t0Width);
    ImGui::SetCursorPos(ImVec2(padding.x + t0Width + padding.x, padding.y));
    toolbars[1].display(windowWidth - t0Width - t2Width - 4 * padding.x);
    ImGui::SetCursorPos(ImVec2(windowWidth - t2Width - padding.x, padding.y));
    toolbars[2].display(t2Width);
}

void Dockspace::toolbarLayoutTwo(Window& window, std::span<int const> spacing) {
    float windowWidth = window.size().x;
    float2 padding = GImGui->Style.WindowPadding;
    float spacer = spacing[0];
    float position[2] = { 0, spacer };
    float width[2] = { spacer, windowWidth - spacer };
    if (spacer >= windowWidth / 2) {
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
            ImVec2(windowWidth - padding.x - t2Width, padding.y));
        toolbars[2].display(t2Width);
    }
}

void Dockspace::toolbarLayoutThree(Window& window,
                                   std::span<int const> spacing) {
    float windowWidth = window.size().x;
    float2 padding = GImGui->Style.WindowPadding;
    float position[3] = { 0, (float)spacing[0], (float)spacing[1] };
    float width[3] = { (float)spacing[0], (float)(spacing[1] - spacing[0]),
                       windowWidth - spacing[1] };
    for (std::size_t i = 0; i < 3; ++i) {
        ImGui::SetCursorPos(ImVec2(position[i] + padding.x, padding.y));
        toolbars[i].display(width[i] - 2 * padding.x);
    }
}

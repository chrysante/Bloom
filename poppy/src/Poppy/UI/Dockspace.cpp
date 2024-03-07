#include "Poppy/UI/Dockspace.hpp"

#include <set>
#include <unordered_set>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <mtl/mtl.hpp>
#include <utl/scope_guard.hpp>
#include <utl/stack.hpp>

#include "Poppy/Core/Debug.hpp"
#include "Poppy/Editor/Editor.hpp"
#include "Poppy/UI/ImGuiHelpers.hpp"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

static constexpr auto MainWindowID = "__MainWindow__";
static constexpr auto MainDockspaceID = "__MainWindow_Dockspace__";

template <typename T>
static void makeUnique(utl::vector<T>& v) {
    auto const last = std::unique(v.begin(), v.end());
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

Dockspace::Dockspace() {}

void Dockspace::display() {
    dockspace();
    displayToolbar();
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

void Dockspace::dockspace() { mainWindow(); }

void Dockspace::mainWindow() {
    ImGuiViewport* const viewport = ImGui::GetMainViewport();
    auto const windowPosition =
        float2(viewport->Pos) + float2(0, toolbars[0].getHeight());
    auto const windowSize =
        float2(viewport->Size) - float2(0, toolbars[0].getHeight());

    ImGui::SetNextWindowPos(windowPosition);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags = 0;
    windowFlags |= ImGuiWindowFlags_MenuBar;
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

    ImGuiID const dockID = ImGui::GetID(MainDockspaceID);
    mainDockID = dockID;

    ImGui::DockSpace(dockID, /* size arg */ {}, flags);
}

utl::small_vector<int, 2> Dockspace::getToolbarSpacing() const {
    utl::vector<mtl::rectangle<int>> const potentialRects = [this] {
        ImGuiDockNode const* const main = ImGui::DockBuilderGetNode(mainDockID);
        assert(main);

        utl::stack<ImGuiDockNode const*> stack;
        stack.push(main);

        utl::vector<mtl::rectangle<int>> result;
        while (stack) {
            auto* node = stack.pop();
            if ((int)node->Pos.y == (int)main->Pos.y && node->IsVisible &&
                node != main && (int2)node->Size != (int2)main->Size)
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
        positions.erase(positions.end() - 1);

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

template <std::invocable Block>
static void toolbarWindow(char const* id, float posX, float2 const size,
                          Block&& block) {
    ImGuiViewport* const viewport = ImGui::GetMainViewport();

    float2 const position = {
        posX,
        viewport->Pos.y + ImGui::FindWindowByName(MainWindowID)->MenuBarHeight()
    };

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

    ImGui::Begin(id, NULL, flags);

    ImGui::PopStyleVar(2);

    block();

    if (ImGui::IsWindowFocused() && GImGui->WindowsFocusOrder.size() > 1 &&
        ImGui::GetIO().MouseReleased[ImGuiMouseButton_Left])
    {
        ImGui::FocusWindow(*(GImGui->WindowsFocusOrder.end() - 2));
    }

    ImGui::End();
}

static void drawSeparator(float positionX) {
    auto* const window = ImGui::GetCurrentWindow();
    auto* const drawList = window->DrawList;

    drawList->AddLine(float2(positionX - 1, window->Pos.y),
                      float2(positionX - 1, window->Pos.y + window->Size.y - 1),
                      ImGui::ColorConvertFloat4ToU32(
                          GImGui->Style.Colors[ImGuiCol_Separator]));
    drawList->AddLine(float2(positionX, window->Pos.y),
                      float2(positionX, window->Pos.y + window->Size.y - 1),
                      ImGui::ColorConvertFloat4ToU32(
                          GImGui->Style.Colors[ImGuiCol_Separator]));
}

void Dockspace::displayToolbar() {
    float const height = toolbars[0].getHeight();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    auto spacing = getToolbarSpacing();
    float const width = viewport->Size.x;
    switch (spacing.size()) {

    case 0:
        break;
    case 1:
        if (spacing[0] >= width * 0.33 &&
            spacing[0] <= width * 0.67) /* in central third */
        {
            spacing.clear();
        }
        break;
    case 2:
        break;
    default:
        BL_DEBUGBREAK();
        break;
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
    assert(spacing.size() <= 2);

    toolbarWindow("toolbar", 0, { viewport->Size.x, height }, [&] {
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
            BL_DEBUGBREAK();
            break;
        }

        // bottom separator
        auto* const window = ImGui::GetCurrentWindow();
        auto* const drawList = window->DrawList;
        drawList->AddLine(float2(0, window->Pos.y + window->Size.y - 1),
                          float2(window->Size.x,
                                 window->Pos.y + window->Size.y - 1),
                          ImGui::ColorConvertFloat4ToU32(
                              GImGui->Style.Colors[ImGuiCol_Separator]));
    });
}

void Dockspace::toolbarLayoutOne(utl::small_vector<int, 2> spacing) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float2 const padding = GImGui->Style.WindowPadding;

    float const t0Width = toolbars[0].getWidthWithoutSpacers();
    float const t2Width = toolbars[2].getWidthWithoutSpacers();
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
    float2 const padding = GImGui->Style.WindowPadding;

    float const spacer = spacing[0];
    float const position[2] = { 0, spacer };

    float const width[2] = { spacer, viewport->Size.x - spacer };

    if (spacer >= viewport->Size.x / 2) {
        // left section is big

        // 1. section
        float const t0Width = toolbars[0].getWidthWithoutSpacers();
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
        // right section is big

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

    drawSeparator(spacer);
}

void Dockspace::toolbarLayoutThree(utl::small_vector<int, 2> spacing) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float2 const padding = GImGui->Style.WindowPadding;

    float const position[3] = { 0, (float)spacing[0], (float)spacing[1] };

    float const width[3] = { (float)spacing[0],
                             (float)(spacing[1] - spacing[0]),
                             viewport->Size.x - spacing[1] };

    for (std::size_t i = 0; i < 3; ++i) {
        ImGui::SetCursorPos(ImVec2(position[i] + padding.x, padding.y));
        toolbars[i].display(width[i] - 2 * padding.x);
    }

    drawSeparator(spacing[0]);
    drawSeparator(spacing[1]);
}

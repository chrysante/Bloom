#include "Poppy/UI/View.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/hashset.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Window.h"
#include "Bloom/Core/Yaml.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace bloom;
using namespace poppy;
using namespace vml::short_types;

namespace {

class IDFactory {
public:
    int makeUnique(int existing) {
        if (existing >= 0 && usedIDs.insert(existing).second) {
            return existing;
        }
        while (usedIDs.contains(IDCounter)) {
            ++IDCounter;
        }
        usedIDs.insert(IDCounter);
        return IDCounter;
    }

private:
    int IDCounter;
    utl::hashset<int> usedIDs;
};

static utl::hashmap<std::string, IDFactory> gIDFactoryMap;

} // namespace

View::View(ViewDescription data):
    data{ .size = data.defaultSize,
          .minSize = data.minSize,
          .padding = { std::nullopt, std::nullopt } } {}

bool View::focused() const { return data.isFocused; }

vml::float2 View::padding() const {
    auto& style = ImGui::GetStyle();
    return {
        data.padding.x.value_or(style.WindowPadding.x),
        data.padding.y.value_or(style.WindowPadding.y),
    };
}

Window& View::window() const {
    BL_ASSERT(
        data.window,
        "Cannot call this method during init(). This should change in the future");
    return *data.window;
}

float2 View::screenSpaceToViewSpace(float2 position) const {
    return position - data.position;
}

float2 View::viewSpaceToScreenSpace(float2 position) const {
    return position + data.position;
}

void View::setFocused() {
    ImGui::FocusWindow((ImGuiWindow*)data.imguiWindow);
    if (data.window) {
        data.window->setFocused();
    }
}

void View::maximize() {
    data.maximized = true;
    // TODO: Send message to window
}

void View::restore() {
    data.maximized = false;
    // TODO: Send message to window
}

void View::toggleMaximize() {
    if (maximized()) {
        restore();
    }
    else {
        maximize();
    }
}

void View::doInit() {
    data.ID = gIDFactoryMap[data.staticName].makeUnique(data.ID);
    if (data.title.empty()) {
        data.title = data.staticName;
    }
    init();
}

static bool isDocked(ImGuiWindow* window) {
    return window && window->DockIsActive;
}

void View::doFrame() {
    auto* imguiWindow = (ImGuiWindow*)data.imguiWindow;
    ImGuiWindowFlags flags = data.nextFrameFlags;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoResize;
    if (_toolbar) {
        flags |= ImGuiWindowFlags_NoTitleBar;
    }
    /// If we are not docked we don't allow movement because otherwise we would
    /// be able to move windows by dragging anywhere in its area. We allow
    /// dragging separately in the title bar area
    if (!isDocked(imguiWindow) && imguiWindow && !imguiWindow->ParentWindow) {
        flags |= ImGuiWindowFlags_NoMove;
    }
    if (toolbar()) {
        flags |= ImGuiWindowFlags_NoDockingTabIndicator;
    }
    // Begin block
    if (!imguiWindow && !data.maximized && !data.docked) {
        ImGui::SetNextWindowPos(data.position);
        ImGui::SetNextWindowSize(data.size);
    }
    if (data.maximized) {
        auto* const viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        flags |= ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoTitleBar;
    }
    else {
        ImGui::SetNextWindowSizeConstraints(data.minSize, { FLT_MAX, FLT_MAX });
    }
    auto& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
                        data.maximized ? 0 : style.WindowRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding());
    if (data.bgColor) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, *data.bgColor);
    }
    data.titleID = utl::strcat(title(), "###", staticName());
    if (data.maximized) {
        data.titleID += "_FS";
    }
    if (data.ID != 0) {
        data.titleID += utl::strcat("_", data.ID);
    }
    bool* beginOpenFlag = isDocked(imguiWindow) ? &data.open : nullptr;
    bool open = ImGui::Begin(data.titleID.c_str(), beginOpenFlag, flags);
    data.nextFrameFlags = 0;
    if (data.bgColor) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar(2);
    if (data.maximized) {
        ImGui::PopStyleVar();
    }
    imguiWindow = ImGui::GetCurrentWindow();
    imguiWindow->UserPointer = this;
    data.imguiWindow = imguiWindow;
    data.isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) &&
                     !ImGui::FindBlockingModal(imguiWindow);
    if (open) {
        data.position = imguiWindow->WorkRect.Min;
        data.size = imguiWindow->Size;
        data.docked = isDocked(imguiWindow);
        auto* viewport = (ImGuiViewportP*)imguiWindow->Viewport;
        data.window = (Window*)viewport->PlatformHandle;
        if (data.window && GImGui->CurrentViewport->Window == imguiWindow) {
            _toolbar ? data.window->addToolbarArea() :
                       data.window->removeToolbarArea();
        }
        if (data.window) {
            displayToolbar();
            display();
        }
        inputState.hovered =
            ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    }
    ImGui::End();
}

/// Copied from Dockspace.cpp
static float computeLeftInset(Window const& window) {
    auto titleButtonsArea = window.titleButtonsArea();
    return titleButtonsArea.lower_bound().x + titleButtonsArea.upper_bound().x;
}

static void dockTitlebarBehaviour(ImGuiWindow* imWindow, ImRect toolbarBB) {
    auto& g = *GImGui;
    bool windowDragging =
        ImGui::IsWindowFocused() &&
        toolbarBB.Contains(g.IO.MouseClickedPos[ImGuiMouseButton_Left]) &&
        !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseDragging(ImGuiMouseButton_Left, 20);
    if (windowDragging) {
        auto* node = imWindow->DockNode;
        ImGui::StartMouseMovingWindowOrNode(imWindow,
                                            node->Windows.Size == 1 ? node :
                                                                      nullptr,
                                            true);
    }
}

void View::displayToolbar() {
    if (!_toolbar) {
        return;
    }
    float2 padding = { 7, 7 };
    _toolbar->setPadding(padding);
    float posx = 0;
    ImGuiWindow* imWindow = (ImGuiWindow*)data.imguiWindow;
    BL_ASSERT(imWindow, "This should not be called if we don't have a window");
    /// We treat the case where we are not docked but don't have a window as if
    /// we are docked, but that should not happen anyways. We could probably
    /// assert that.
    bool docked = data.docked || !data.window;
    if (docked) {
        _toolbar->setHeight(25);
    }
    else {
        posx += computeLeftInset(window());
        float titlebarHeight = window().titlebarHeight();
        _toolbar->setHeight(titlebarHeight - 2 * padding.y);
        window().setMovable(!ImGui::IsAnyItemHovered());
    }
    ImGui::SetCursorPosX(posx);
    float posy = ImGui::GetCursorPosY();
    imWindow->DC.CursorPos =
        (float2)imWindow->Pos + float2(posx, posy) - this->padding();
    _toolbar->display();
    ImGui::SetCursorPos({ 0, posy + _toolbar->getHeight() + 1 });
    if (docked) {
        float2 pos = ImGui::GetWindowPos();
        ImRect toolbarRect(pos, pos + float2(ImGui::GetWindowWidth(),
                                             _toolbar->getHeight()));
        dockTitlebarBehaviour(imWindow, toolbarRect);
    }
}

void View::doOnInput(bloom::InputEvent& event) {
    using enum InputEventMask;
    event.dispatch<MouseDown>(
        [&](MouseEvent const&) { inputState.capture = inputState.hovered; });
    event.dispatch<MouseUp>(
        [&](MouseEvent const&) { inputState.capture = false; });
    if (test(event.type() & (MouseDragged | ScrollWheel | Magnify)) &&
        !(inputState.hovered || inputState.capture))
    {
        return;
    }
    if (test(event.type() & Key) && !focused()) {
        return;
    }
    cmdPalette.onInput(event);
    if (event.handled()) {
        return;
    }
    onInput(event);
}

YAML::Node View::doSerialize() const {
    YAML::Node node;
    node["ID"] = data.ID;
    node["Position"] = data.position;
    node["Size"] = data.size;
    node["Docked"] = data.docked;
    node["Static Name"] = data.staticName;
    node["Derived Data"] = serialize();
    return node;
}

std::unique_ptr<View> View::doDeserialize(YAML::Node const& node,
                                          Editor& editor) {
    try {
        int ID = node["ID"].as<int>();
        std::string staticName = node["Static Name"].as<std::string>();
        if (ID < 0) {
            throw std::runtime_error("Invalid View ID");
        }
        auto entry = ViewRegistry::get(staticName);
        if (!entry) {
            Logger::Error("Failed to deserialize view: unknown static name: ",
                          staticName);
            return nullptr;
        }
        auto view = entry->createView(editor);
        view->data.ID = ID;
        view->data.position = node["Position"].as<float2>();
        view->data.size = node["Size"].as<float2>();
        view->data.docked = node["Docked"].as<bool>();
        view->deserialize(node["Derived Data"]);
        return view;
    }
    catch (std::exception const& e) {
        Logger::Error(e.what());
    }
    return nullptr;
}

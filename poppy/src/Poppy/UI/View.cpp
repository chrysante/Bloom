#include "Poppy/UI/View.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/hashset.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

#include "Bloom/Application/Window.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace bloom;
using namespace poppy;
using namespace vml::short_types;

namespace {

class IDFactory {
public:
    static int getNewID() {
        while (instance.usedIDs.contains(instance.IDCounter)) {
            ++instance.IDCounter;
        }
        instance.usedIDs.insert(instance.IDCounter);
        return instance.IDCounter;
    }

private:
    static IDFactory instance;

    int IDCounter;
    utl::hashset<int> usedIDs;
};

IDFactory IDFactory::instance;

} // namespace

View::View(ViewDescription data):
    data{ .size = data.defaultSize,
          .minSize = data.minSize,
          .padding = { std::nullopt, std::nullopt } } {}

bool View::focused() const {
    auto* w = GImGui->NavWindow;
    while (w) {
        if (w == data.imguiWindow) {
            return true;
        }
        w = w->ParentWindow;
    }
    return false;
}

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

float2 View::windowSpaceToViewSpace(float2 position) const {
    return position - data.position + data.window->position();
}

float2 View::viewSpaceToWindowSpace(float2 position) const {
    return position + data.position - data.window->position();
}

void View::setFocused() { ImGui::FocusWindow((ImGuiWindow*)data.imguiWindow); }

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
    if (data.ID < 0) {
        data.ID = IDFactory::getNewID();
    }
    if (data.title.empty()) {
        data.title = data.staticName;
    }
    init();
}

static bool isDocked(ImGuiWindow* window) {
    return window && window->ParentWindow;
}

void View::doFrame() {
    auto* imguiWindow = (ImGuiWindow*)data.imguiWindow;
    if (isDocked(imguiWindow)) {
        data.isMoveable = true;
    }
    else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        data.isMoveable = false;
    }
    if (imguiWindow) {
        imguiWindow->UserPointer = this;
    }
    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoCollapse;
    flags |= ImGuiWindowFlags_NoResize;
    if (_toolbar) {
        flags |= ImGuiWindowFlags_NoTitleBar;
    }
    if (!data.isMoveable) {
        flags |= ImGuiWindowFlags_NoMove;
    }
    // Begin block
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
        ImGui::SetNextWindowSize(data.size);
        ImGui::SetNextWindowSizeConstraints(data.minSize, { FLT_MAX, FLT_MAX });
    }
    auto& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
                        data.maximized ? 0 : style.WindowRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding());
    if (data.bgColor) {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, *data.bgColor);
    }
    std::string displayTitle =
        utl::strcat(title(), "##", data.staticName, "_", data.ID);
    if (data.maximized) {
        displayTitle += "_FS";
    }
    bool* beginOpenFlag = isDocked(imguiWindow) ? &data.open : nullptr;
    bool open = ImGui::Begin(displayTitle.data(), beginOpenFlag, flags);
    if (data.bgColor) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar(2);
    if (data.maximized) {
        ImGui::PopStyleVar();
    }
    if (open) {
        auto* imguiWindow = ImGui::GetCurrentWindow();
        data.imguiWindow = imguiWindow;
        data.size = imguiWindow->Size;
        data.position = imguiWindow->Pos;
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
    }
    ImGui::End();
}

/// Copied from Dockspace.cpp
static float computeLeftInset(Window const& window) {
    auto titleButtonsArea = window.titleButtonsArea();
    return titleButtonsArea.lower_bound().x + titleButtonsArea.upper_bound().x;
}

static bool mouseInZoomArea(ImGuiWindow* imWindow, float titlebarHeight) {
    bool windowHovered =
        GImGui->HoveredWindow &&
        ImGui::IsWindowChildOf(GImGui->HoveredWindow, imWindow, false, false);
    return windowHovered && !ImGui::IsAnyItemHovered() &&
           ImGui::GetMousePos().y - imWindow->Pos.y <= titlebarHeight;
}

void View::displayToolbar() {
    if (!_toolbar) {
        return;
    }
    float2 padding = { 6, 6 };
    _toolbar->setPadding(padding);
    float posx = 0;
    ImGuiWindow* imWindow = (ImGuiWindow*)data.imguiWindow;
    if (!isDocked(imWindow) && data.window) {
        posx += computeLeftInset(window());
        float titlebarHeight = window().titlebarHeight();
        _toolbar->setHeight(titlebarHeight - 2 * padding.y);
        window().setMovable(!ImGui::IsAnyItemHovered());
        if (mouseInZoomArea(imWindow, titlebarHeight) &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            window().zoom();
        }
    }
    else {
        _toolbar->setHeight(26);
    }
    ImGui::SetCursorPosX(posx);
    float posy = ImGui::GetCursorPosY();
    _toolbar->display();
    ImGui::SetCursorPos({ 0, posy + _toolbar->getHeight() + 1 });
}

void View::doOnInput(bloom::InputEvent& event) {
    if (focused()) {
        cmdPalette.onInput(event);
        if (event.handled()) {
            return;
        }
    }
    onInput(event);
}

YAML::Node View::doSerialize() const {
    YAML::Node node;
    node["ID"] = data.ID;
    node["Static Name"] = data.staticName;
    node["Derived Data"] = serialize();
    return node;
}

std::unique_ptr<View> View::doDeserialize(YAML::Node const& node) {
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
        auto view = entry->createView();
        view->data.ID = ID;
        view->deserialize(node["Derived Data"]);
        return view;
    }
    catch (std::exception const& e) {
        Logger::Error(e.what());
    }
    return nullptr;
}

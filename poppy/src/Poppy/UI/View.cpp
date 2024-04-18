#include "Poppy/UI/View.h"

#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/hashset.hpp>
#include <utl/scope_guard.hpp>
#include <utl/strcat.hpp>

using namespace bloom;
using namespace vml::short_types;
using namespace poppy;

static int gID = 0;
static utl::hashset<int> usedIDs;

static int getFreshID() {
    while (usedIDs.contains(gID)) {
        ++gID;
    }
    return gID;
}

bool View::focused() const {
    auto* w = GImGui->NavWindow;
    while (w) {
        if (w == desc.imguiWindow) {
            return true;
        }
        w = w->ParentWindow;
    }
    return false;
}

vml::float2 View::windowSpaceToViewSpace(vml::float2 position) const {
    return position - desc.position;
}

vml::float2 View::viewSpaceToWindowSpace(vml::float2 position) const {
    return position + desc.position;
}

void View::setFocused() { ImGui::FocusWindow((ImGuiWindow*)desc.imguiWindow); }

void View::maximize() {
    desc.maximized = true;
    // send message to window
}

void View::restore() {
    desc.maximized = false;
    // send message to window
}

void View::toggleMaximize() {
    if (maximized()) {
        restore();
    }
    else {
        maximize();
    }
}

void View::setPadding(vml::float2 padding) {
    desc.hasPaddingX = padding.x >= 0;
    desc.hasPaddingY = padding.y >= 0;

    desc.pub.padding = padding;
}

void View::setTitle(std::string newTitle) {
    desc.pub.title = std::move(newTitle);
}

void View::doInit() {
    if (desc.pub.id < 0) {
        desc.pub.id = getFreshID();
    }
    usedIDs.insert(desc.pub.id);
    setPadding(desc.pub.padding);

    desc.pub.title = std::string(name());

    this->init();
}

void View::doShutdown() { this->shutdown(); }

void View::doFrame() {
    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoCollapse;
    // Begin block
    if (desc.maximized) {
        auto* const viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowPos({ 0, 0 });
        flags |= ImGuiWindowFlags_NoTitleBar;
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoResize;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    }
    auto& style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,
                        desc.maximized ? 0 : style.WindowRounding);
    vml::float2 padding = {
        desc.hasPaddingX ? desc.pub.padding.x : style.WindowPadding.x,
        desc.hasPaddingY ? desc.pub.padding.y : style.WindowPadding.y
    };
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    std::string displayTitle = utl::strcat(title(), "###", name(), "-", id());
    if (desc.maximized) {
        displayTitle += "-FS";
    }
    bool const open = ImGui::Begin(displayTitle.data(), &desc.open, flags);
    ImGui::PopStyleVar(2);
    if (desc.maximized) {
        ImGui::PopStyleVar();
    }
    if (open) {
        auto* window = ImGui::GetCurrentWindow();
        desc.imguiWindow = window;
        desc.pub.size = window->Size;
        desc.position = window->Pos;
        desc.windowSize = ImGui::GetMainViewport()->Size;
        frame();
    }
    ImGui::End();
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
    node["View Base"] = desc.pub;
    node["Derived View"] = serialize();
    return node;
}

std::unique_ptr<View> View::doDeserialize(YAML::Node const& node) {
    try {
        ViewDescription desc = node["View Base"].as<ViewDescription>();
        BL_ASSERT(desc.id >= 0, "Invalid View ID");
        auto entry = ViewRegistry::get(desc.name);
        if (!entry) {
            Logger::Error("Failed to deserialize '", desc.name, "'");
            return nullptr;
        }
        auto result = entry->factory();
        ViewDescPrivate privDesc{ desc };
        result->desc = privDesc;
        result->deserialize(node["Derived View"]);
        result->mRegisterDescription = entry->description;
        return result;
    }
    catch (std::exception const& e) {
        Logger::Error(e.what());
    }
    return nullptr;
}

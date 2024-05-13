#include "Poppy/UI/ViewManager.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/algorithm.hpp>
#include <vml/vml.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

struct ViewManager::Impl {
    static void populate(View& view, Editor& editor) {
        view.data.editor = &editor;
        view.assignEmitter(editor.makeEmitter());
        view.assignReceiver(editor.makeReceiver());
    }

    static void displayDebugInfo(View const& view) {
        auto* imWindow = (ImGuiWindow*)view.data.imguiWindow;
        auto* window = view.data.window;
        if (!imWindow || !window) {
            return;
        }
        ImGui::Begin(view.data.titleID.c_str());
        ImGui::SetCursorPos({});
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0);
        ImGui::BeginChild("Debug Overlay", {}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_NoInputs);
        ImGui::PopStyleColor();
        auto* DL = ImGui::GetWindowDrawList();
        float2 pos =
            (float2)imWindow->Pos + (float2)ImGui::GetStyle().WindowPadding;
        pos.y += window->titlebarHeight();
        ImU32 color = IM_COL32(255, 0, 255, 255);
        auto field = [&](std::string_view label, auto const&... args) {
            DL->AddText(pos, color, utl::strcat(label, ": ", args...).c_str());
            pos.y += ImGui::GetFontSize();
        };
        field("System window position: ", window->position());
        field("System window size: ", window->size());
        field("ImGui window position: ", (float2)imWindow->Pos);
        field("ImGui window size: ", (float2)imWindow->Size);
        field("Title: ", view.data.title);
        field("Docked: ", view.data.docked ? "YES" : "NO");
        field("Padding X: ", view.data.padding.x ?
                                 utl::strcat(*view.data.padding.x) :
                                 "Default");
        field("Padding X: ", view.data.padding.y ?
                                 utl::strcat(*view.data.padding.y) :
                                 "Default");
        field("Is focused: ", view.data.isFocused ? "YES" : "NO");
        ImGui::EndChild();
        ImGui::End();
    }
};

ViewManager::ViewManager(Editor* editor, Receiver receiver):
    Receiver(std::move(receiver)), editor(editor) {
    listen([this](OpenCommand const& cmd) { openView(cmd.name); });
    listen([this](CloseAllCommand) { closeAll(); });
    listen([this](ToggleDebugView) { displayDebugger ^= true; });
}

ViewManager::~ViewManager() = default;

View* ViewManager::openView(ViewRegistryEntry const& regEntry) {
    views.push_back(regEntry.createView());
    auto* view = views.back().get();
    Impl::populate(*view, *editor);
    view->doInit();
    return view;
}

View* ViewManager::openView(std::string_view name) {
    if (auto* view = findByStaticName(name)) {
        view->setFocused();
        return view;
    }
    auto regEntry = ViewRegistry::get(name);
    if (!regEntry) {
        Logger::Error("Failed to create View (does not exist): ", name);
    }
    return openView(*regEntry);
}

View* ViewManager::findByStaticName(std::string_view name) {
    auto itr = ranges::find_if(views, [&](auto& view) {
        return view->staticName() == name;
    });
    return itr != views.end() ? itr->get() : nullptr;
}

void ViewManager::closeAll() { views.clear(); }

void ViewManager::display() {
    /// Clear closing views
    views.erase(ranges::remove_if(views,
                                  [](auto& view) { return !view->data.open; }),
                views.end());
    /// If we have a maximized view we only display that. This is wrong because
    /// for multiple displays it would hide the views on other displays but we
    /// leave it for now
    auto max = ranges::find_if(views, [](auto&& v) { return v->maximized(); });
    if (max != views.end()) {
        auto& maximizedView = **max;
        maximizedView.doFrame();
        return;
    }
    for (auto& view: views) {
        view->doFrame();
        if (displayDebugger) {
            Impl::displayDebugInfo(*view);
        }
    }
}

bool ViewManager::onInput(InputEvent& event) {
    /// Give to focused view
    auto focused = ranges::find_if(views, [](auto& v) { return v->focused(); });
    if (focused != views.end()) {
        (*focused)->doOnInput(event);
        if (event.handled()) {
            return true;
        }
    }
    /// Give to all unfocused views
    for (auto& view: views) {
        if (view->focused()) {
            continue;
        }
        view->doOnInput(event);
        if (event.handled()) {
            return true;
        }
    }
    return false;
}

YAML::Node ViewManager::serialize() const {
    YAML::Node root;
    for (auto& view: views) {
        root.push_back(view->doSerialize());
    }
    return root;
}

void ViewManager::deserialize(YAML::Node const& node) {
    views.clear();
    for (auto viewData: node) {
        auto owner = View::doDeserialize(viewData);
        if (!owner) {
            continue;
        }
        views.push_back(std::move(owner));
        auto* view = views.back().get();
        Impl::populate(*view, *editor);
        view->doInit();
    }
}

#include "Poppy/UI/ViewManager.h"

#include <range/v3/algorithm.hpp>

#include "Bloom/Application/InputEvent.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace poppy;
using namespace bloom;

struct ViewManager::Impl {
    static void populate(View& view, Editor& editor) {
        view.data.editor = &editor;
        view.assignEmitter(editor.makeEmitter());
        view.assignReceiver(editor.makeReceiver());
    }
};

ViewManager::ViewManager(Editor* editor): editor(editor) {}

ViewManager::~ViewManager() = default;

View* ViewManager::openView(ViewRegistryEntry const& regEntry) {
    views.push_back(regEntry.createView());
    auto* view = views.back().get();
    Impl::populate(*view, *editor);
    view->doInit();
    return view;
}

View* ViewManager::openView(std::string_view name) {
    auto regEntry = ViewRegistry::get(name);
    if (!regEntry) {
        Logger::Error("Failed to create View: ", name);
    }
    return openView(*regEntry);
}

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

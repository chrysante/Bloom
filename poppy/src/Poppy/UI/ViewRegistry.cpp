#include "Poppy/UI/ViewRegistry.h"

#include "Bloom/Core/Debug.h"
#include "Poppy/UI/View.h"

using namespace poppy;
using namespace bloom;

std::unique_ptr<View> ViewRegistryEntry::createView() const {
    auto view = factory();
    view->data.staticName = name();
    return view;
}

void ViewRegistry::add(ViewRegistryEntry entry) {
    bool success = instance()._entries.insert({ entry.name(), entry }).second;
    if (!success) {
        Logger::Error("View is already registered: ", entry.name());
    }
}

std::optional<ViewRegistryEntry> ViewRegistry::get(std::string_view name) {
    auto& inst = instance();
    auto itr = inst._entries.find(name);
    if (itr == inst._entries.end()) {
        return std::nullopt;
    }
    return itr->second;
}

ViewRegistry& ViewRegistry::instance() {
    static ViewRegistry instance;
    return instance;
}

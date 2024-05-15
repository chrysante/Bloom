#ifndef POPPY_UI_VIEWREGISTRY_H
#define POPPY_UI_VIEWREGISTRY_H

#include <functional>
#include <memory>
#include <string>

#include <range/v3/view.hpp>
#include <utl/common.hpp>
#include <utl/hashtable.hpp>

#include "Poppy/UI/Fwd.h"

/// Registers the view class \p ViewClass with the registry
#define POPPY_REGISTER_VIEW(ViewClass, Name)                                   \
    UTL_STATIC_INIT {                                                          \
        ViewRegistry::add(                                                     \
            { Name, [] { return std::make_unique<ViewClass>(); } });           \
    }

namespace poppy {

class Editor;

/// Entry in the view manager that represents a concrete view class
class ViewRegistryEntry {
public:
    using Factory = std::function<std::unique_ptr<View>()>;

    ViewRegistryEntry(std::string name, Factory factory):
        _name(std::move(name)), factory(std::move(factory)) {}

    /// \Returns the name of the view the entry manages
    std::string const& name() const { return _name; }

    /// Create an empty view object of this view type
    std::unique_ptr<View> createView(Editor& editor) const;

private:
    std::string _name;
    Factory factory;
};

/// Static class that manages the view classes in the application
class ViewRegistry {
public:
    /// Adds \p entry to the registry
    static void add(ViewRegistryEntry entry);

    /// \Returns the entry with name \p name if it exists
    static std::optional<ViewRegistryEntry> get(std::string_view name);

    /// \Returns a view over all entries in the registry
    static auto entries() {
        return instance()._entries.values() | ranges::views::values;
    }

private:
    ViewRegistry() = default;

    static ViewRegistry& instance();

    utl::hashmap<std::string, ViewRegistryEntry> _entries;
};

} // namespace poppy

#endif // POPPY_UI_VIEWREGISTRY_H

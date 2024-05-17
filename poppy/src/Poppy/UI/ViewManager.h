#ifndef POPPY_UI_VIEWMANAGER_H
#define POPPY_UI_VIEWMANAGER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Bloom/Application/Fwd.h"
#include "Bloom/Application/MessageSystem.h"
#include "Bloom/Core/Yaml.h"
#include "Poppy/UI/Fwd.h"

namespace poppy {

class Editor;

/// Creates and destroys all view instances in the application
class ViewManager: bloom::Receiver {
public:
    struct ToggleDebugView {};

    struct OpenCommand {
        std::string name;
        View** result = nullptr;
        std::function<void(View&)> completion = {};
    };
    struct CloseAllCommand {};

    explicit ViewManager(Editor* editor, Receiver receiver);
    ViewManager(ViewManager const&) = delete;
    ViewManager& operator=(ViewManager const&) = delete;
    ~ViewManager();

    /// Creates an instance of the view \p entry
    View* openView(ViewRegistryEntry const& entry);

    /// Focuses the existing view with static name \p name or, if nonesuch
    /// exists, finds the view registry entry with name \p name and creates a
    /// new view
    View* openView(std::string_view name,
                   std::function<void(View&)> completion = {});

    /// Finds and returns the first open view with static name \p name or null
    /// if none exists
    View* findByStaticName(std::string_view name);

    /// Closes all open views
    void closeAll();

    /// Displays all views
    void display();

    /// Passes the \p event to all views
    bool onInput(bloom::InputEvent& event);

    /// Serializes the currently open views
    YAML::Node serialize() const;

    /// Recreates the views stored in \p node
    void deserialize(YAML::Node const& node);

private:
    struct Impl;
    Editor* editor; // To create emitters and receivers
    std::vector<std::unique_ptr<View>> views;
    bool displayDebugger = false;
};

} // namespace poppy

#endif // POPPY_UI_VIEWMANAGER_H

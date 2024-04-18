#ifndef POPPY_UI_COMMANDPALETTE_H
#define POPPY_UI_COMMANDPALETTE_H

#include <functional>
#include <string>

#include <utl/hashtable.hpp>

#include "Bloom/Application/Input.h"
#include "Bloom/Application/InputEvent.h"

namespace poppy {

///
struct KeyCombination {
    ///
    bloom::Key key;

    ///
    bloom::ModFlags modFlags;
};

/// Set of commands
class CommandPalette {
public:
    ///
    class Action {
    public:
        Action(std::function<void()> fn, std::function<bool()> enabled):
            fn(std::move(fn)), _enabled(std::move(enabled)) {}

        /// Performs the command
        /// \Pre `enabled()` must return `true`
        void execute() const;

        /// \Returns `true` if this command can be executed
        bool enabled() const { return !_enabled || _enabled(); }

    private:
        std::function<void()> fn;
        std::function<bool()> _enabled;
    };

    /// Inserts a command.
    ///
    /// \p keyComb is canonicalized depending on the OS. This means that on
    /// Apple systems the control modifier is mapped to cmd
    void add(std::string name, KeyCombination keyComb,
             std::function<void()> action, std::function<bool()> enabled = {});

    /// Executes the command \p name if a matching command exists
    void doCommand(std::string_view name) const;

    ///
    Action const* find(std::string_view name) const;

private:
    struct ModActionPair {
        bloom::ModFlags modFlags;
        Action action;
    };

    friend class Editor; // To call onInput()
    friend class View;   // To call onInput()

    void onInput(bloom::InputEvent& event) const;

    utl::hashmap<std::string, Action> nameMap;
    utl::hashmap<bloom::Key, std::vector<ModActionPair>> keyMap;
};

} // namespace poppy

#endif // POPPY_UI_COMMANDPALETTE_H

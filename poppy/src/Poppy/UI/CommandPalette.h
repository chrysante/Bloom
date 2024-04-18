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

/// Element type of the command palette. A command is a named action
class Command {
public:
    Command(std::string name, std::function<void()> fn,
            std::function<bool()> enabled):
        _name(std::move(name)),
        fn(std::move(fn)),
        _enabled(std::move(enabled)) {}

    /// \Returns the name of this command
    std::string const& name() const { return _name; }

    /// Performs the command
    /// \Pre `enabled()` must return `true`
    void execute() const;

    /// \Returns `true` if this command can be executed
    bool enabled() const { return !_enabled || _enabled(); }

private:
    std::string _name;
    std::function<void()> fn;
    std::function<bool()> _enabled;
};

/// Set of commands
class CommandPalette {
public:
    /// Inserts a command.
    ///
    /// \p keyComb is canonicalized depending on the OS. This means that on
    /// Apple systems the control modifier is mapped to cmd
    void add(std::string name, KeyCombination keyComb,
             std::function<void()> action, std::function<bool()> enabled = {});

    /// Executes the command \p name if a matching command exists
    void doCommand(std::string_view name) const;

    ///
    Command const* find(std::string_view name) const;

private:
    struct ModCommandPair {
        bloom::ModFlags modFlags;
        Command command;
    };

    friend class Editor; // To call onInput()
    friend class View;   // To call onInput()

    void onInput(bloom::InputEvent& event) const;

    /// This is a node hashmap because we give out pointers to elements
    utl::node_hashmap<std::string, Command> nameMap;
    utl::hashmap<bloom::Key, std::vector<ModCommandPair>> keyMap;
};

} // namespace poppy

#endif // POPPY_UI_COMMANDPALETTE_H

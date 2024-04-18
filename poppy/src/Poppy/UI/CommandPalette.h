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
    /// Inserts a command.
    ///
    /// \p keyComb is canonicalized depending on the OS. This means that on
    /// Apple systems the control modifier is mapped to cmd
    void add(std::string name, KeyCombination keyComb,
             std::function<void()> action);

    /// Executes the command \p name if a matching command exists
    void doCommand(std::string_view name) const;

private:
    using Action = std::function<void()>;

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

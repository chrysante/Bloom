#include "Poppy/UI/CommandPalette.h"

#include <bit>

#include <range/v3/algorithm.hpp>

#include "Bloom/Core/Debug.h"

using namespace poppy;
using namespace bloom;

void Command::execute() const {
    BL_EXPECT(enabled());
    fn();
}

static ModFlags canonicalize(ModFlags mods) {
    using enum ModFlags;
#if defined(__APPLE__)
    if (test(mods & Control)) {
        mods &= ~Control;
        mods |= Super;
    }
#endif
    return mods;
}

Command const* CommandPalette::add(std::string name, KeyCombination keyComb,
                                   std::function<void()> fn,
                                   std::function<bool()> enabled) {
    return add({ std::move(name), keyComb, std::move(fn), std::move(enabled) });
}

Command const* CommandPalette::add(Command cmd) {
    cmd.keyComb.modFlags = canonicalize(cmd.keyComb.modFlags);
    auto* result = &nameMap.insert({ cmd.name(), cmd }).first->second;
    auto keyComb = cmd.keyCombination();
    if (keyComb.key != Key::None) {
        auto& list = keyMap[keyComb.key];
        list.push_back({ canonicalize(keyComb.modFlags), cmd });
    }
    return result;
}

void CommandPalette::doCommand(std::string_view name) const {
    auto* cmd = find(name);
    if (!cmd) {
        Logger::Warn("Command not found: \"", name, "\"");
        return;
    }
    cmd->execute();
}

Command const* CommandPalette::find(std::string_view name) const {
    auto itr = nameMap.find(name);
    if (itr == nameMap.end()) {
        return nullptr;
    }
    return &itr->second;
}

void CommandPalette::onInput(InputEvent& event) const {
    event.dispatch<InputEventMask::KeyDown>([&](KeyEvent const& event) {
        auto listItr = keyMap.find(event.key);
        if (listItr == keyMap.end()) {
            return false;
        }
        auto& list = listItr->second;
        /// Sometimes numlock gets set even if not held
        auto modFlags = event.modifierFlags & ~ModFlags::NumLock;
        auto itr = ranges::find_if(list, [&](auto& p) {
            return p.modFlags == modFlags;
        });
        if (itr == list.end()) {
            return false;
        }
        if (itr->command.enabled()) {
            itr->command.execute();
        }
        return true;
    });
}

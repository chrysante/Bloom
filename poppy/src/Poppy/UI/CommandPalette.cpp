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

static constexpr auto ModFlagOrder = [](ModFlags a, ModFlags b) {
    return std::popcount((size_t)a) > std::popcount((size_t)b);
};

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

void CommandPalette::add(std::string name, KeyCombination keyComb,
                         std::function<void()> fn,
                         std::function<bool()> enabled) {
    Command command = { name, std::move(fn), std::move(enabled) };
    nameMap.insert({ name, command });
    auto& list = keyMap[keyComb.key];
    if (keyComb.key != Key::None) {
        list.push_back({ canonicalize(keyComb.modFlags), command });
        ranges::sort(list, ModFlagOrder, &ModCommandPair::modFlags);
    }
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
    event.dispatch<InputEventMask::KeyUp>([&](KeyEvent const& event) {
        auto listItr = keyMap.find(event.key);
        if (listItr == keyMap.end()) {
            return false;
        }
        auto& list = listItr->second;
        auto itr = ranges::find_if(list, [&](auto& p) {
            return test(p.modFlags & event.modifierFlags);
        });
        if (itr == list.end()) {
            return false;
        }
        itr->command.execute();
        return true;
    });
}

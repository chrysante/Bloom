#include "Poppy/UI/CommandPalette.h"

#include <bit>

#include <range/v3/algorithm.hpp>

#include "Bloom/Core/Debug.h"

using namespace poppy;
using namespace bloom;

using Action = CommandPalette::Action;

void Action::execute() const {
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
    Action action = { std::move(fn), std::move(enabled) };
    nameMap.insert({ std::move(name), action });
    auto& list = keyMap[keyComb.key];
    if (keyComb.key != Key::None) {
        list.push_back({ canonicalize(keyComb.modFlags), action });
        ranges::sort(list, ModFlagOrder, &ModActionPair::modFlags);
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

Action const* CommandPalette::find(std::string_view name) const {
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
        itr->action.execute();
        return true;
    });
}

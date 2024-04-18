#include "Poppy/UI/CommandPalette.h"

#include <bit>

#include <range/v3/algorithm.hpp>

using namespace poppy;
using namespace bloom;

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
                         std::function<void()> action) {
    nameMap.insert({ std::move(name), action });
    auto& list = keyMap[keyComb.key];
    if (keyComb.key != Key::None) {
        list.push_back({ canonicalize(keyComb.modFlags), action });
        ranges::sort(list, ModFlagOrder, &ModActionPair::modFlags);
    }
}

void CommandPalette::doCommand(std::string_view name) const {
    auto itr = nameMap.find(name);
    if (itr != nameMap.end()) {
        itr->second();
    }
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
        itr->action();
        return true;
    });
}

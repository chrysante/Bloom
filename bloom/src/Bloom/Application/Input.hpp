#pragma once

#include "Bloom/Core/Base.hpp"

#include <array>

#include <mtl/mtl.hpp>
#include <utl/common.hpp>

namespace bloom {

enum class ModFlags : unsigned {
    none     = 0u,
    capsLock = 1u << 0, // Set if Caps Lock key is pressed.
    shift    = 1u << 1, // Set if Shift key is pressed.
    control  = 1u << 2, // Set if Control key is pressed.
    alt      = 1u << 3, // Set if Alt key is pressed.
    super    = 1u << 4, // Set if Command key is pressed.
    numLock  = 1u << 5, // Set if numLock ...
};

UTL_ENUM_OPERATORS(ModFlags);

ModFlags modFlagsFromGLFW(int);

enum class BLOOM_API Key {
    none = 0,
    A,
    S,
    D,
    F,
    H,
    G,
    Z,
    X,
    C,
    V,
    B,
    Q,
    W,
    E,
    R,
    Y,
    T,
    O,
    U,
    I,
    P,
    L,
    J,
    K,
    N,
    M,
    _1,
    _2,
    _3,
    _4,
    _5,
    _6,
    _7,
    _8,
    _9,
    _0,
    equal,
    minus,
    rightBracket,
    leftBracket,
    apostrophe,
    semicolon,
    backslash,
    comma,
    slash,
    period,
    graveAccent,
    keypadDecimal,
    keypadMultiply,
    keypadAdd,
    numLock,
    keypadDivide,
    keypadEnter,
    keypadSubtract,
    keypadEqual,
    keypad0,
    keypad1,
    keypad2,
    keypad3,
    keypad4,
    keypad5,
    keypad6,
    keypad7,
    keypad8,
    keypad9,
    enter,
    tab,
    space,
    backspace,
    escape,
    capsLock,
    leftCtrl,
    leftShift,
    leftAlt,
    leftSuper,
    rightCtrl,
    rightShift,
    rightAlt,
    rightSuper,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    printScreen,
    menu,
    insert,
    home,
    pageUp,
    delete_,
    end,
    pageDown,
    leftArrow,
    rightArrow,
    downArrow,
    upArrow,
    _count
};

Key keyFromGLFW(int);

enum struct BLOOM_API MouseButton {
    none  = -1,
    left  = 0,
    right = 1,
    other = 2,
    _count
};

MouseButton mouseButtonFromGLFW(int);

class InputEvent;

class BLOOM_API Input {
    friend class Window;

public:
    mtl::float2 mousePosition() const { return _mousePosition; }
    mtl::float2 mouseOffset() const { return _mouseOffset; }

    mtl::float2 scrollOffset() const { return _scrollOffset; }

    bool mouseDown(MouseButton button) const {
        return _mouseButtons[(std::size_t)button];
    }
    bool keyDown(Key key) const { return _keys[(std::size_t)key]; }
    int keyDownRepeatCount(Key key) const { return _keys[(std::size_t)key]; }

    ModFlags modFlags() const { return _modFlags; }

private:
    void _setKey(Key k, bool down) { _keys[(std::size_t)k] = down; }
    void _setMouseButton(MouseButton b, bool down) {
        _mouseButtons[(std::size_t)b] = down;
    }

private:
    ModFlags _modFlags         = ModFlags::none;
    mtl::float2 _mousePosition = 0;
    mtl::float2 _mouseOffset   = 0;
    mtl::float2 _scrollOffset  = 0;
    std::array<int, (std::size_t)Key::_count> _keys{};
    std::array<int, (std::size_t)MouseButton::_count> _mouseButtons{};
};

} // namespace bloom

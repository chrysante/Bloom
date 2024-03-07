#ifndef BLOOM_APPLICATION_INPUT_H
#define BLOOM_APPLICATION_INPUT_H

#include <array>

#include <mtl/mtl.hpp>
#include <utl/common.hpp>

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/EnumCount.hpp"

namespace bloom {

/// Flags type to indicate if modifier keys are pressed
enum class ModFlags : unsigned {
    None = 0u,
    CapsLock = 1u << 0, /// Set if caps lock key is pressed.
    Shift = 1u << 1,    /// Set if shift key is pressed.
    Control = 1u << 2,  /// Set if control key is pressed.
    Alt = 1u << 3,      /// Set if alt key is pressed.
    Super = 1u << 4,    /// Set if command key is pressed.
    NumLock = 1u << 5,  /// Set if numLock key is pressed.
};

UTL_ENUM_OPERATORS(ModFlags);

/// Translates GLFW modifier flags to `bloom::ModFlags`
ModFlags modFlagsFromGLFW(int glfwModFlags);

/// All known keyboard keys
enum class BLOOM_API Key {
    None = 0,
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
    Equal,
    Minus,
    RightBracket,
    LeftBracket,
    Apostrophe,
    Semicolon,
    Backslash,
    Comma,
    Slash,
    Period,
    GraveAccent,
    KeypadDecimal,
    KeypadMultiply,
    KeypadAdd,
    NumLock,
    KeypadDivide,
    KeypadEnter,
    KeypadSubtract,
    KeypadEqual,
    Keypad0,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    Enter,
    Tab,
    Space,
    Backspace,
    Escape,
    CapsLock,
    LeftCtrl,
    LeftShift,
    LeftAlt,
    LeftSuper,
    RightCtrl,
    RightShift,
    RightAlt,
    RightSuper,
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
    PrintScreen,
    Menu,
    Insert,
    Home,
    PageUp,
    Delete,
    End,
    PageDown,
    LeftArrow,
    RightArrow,
    DownArrow,
    UpArrow,
    LAST = UpArrow
};

/// Translates GLFW key IDs to `bloom::Key`
Key keyFromGLFW(int);

enum struct BLOOM_API MouseButton {
    None = -1,
    Left = 0,
    Right = 1,
    Other = 2,
    LAST = Other
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
    ModFlags _modFlags = ModFlags::None;
    mtl::float2 _mousePosition = 0;
    mtl::float2 _mouseOffset = 0;
    mtl::float2 _scrollOffset = 0;
    std::array<int, EnumCount<Key>> _keys{};
    std::array<int, EnumCount<MouseButton>> _mouseButtons{};
};

} // namespace bloom

#endif // BLOOM_APPLICATION_INPUT_H

#ifndef BLOOM_APPLICATION_INPUT_H
#define BLOOM_APPLICATION_INPUT_H

#include <array>

#include <utl/common.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/EnumCount.h"

namespace bloom {

class InputEvent;

/// Flags type to indicate if modifier keys are pressed
enum class ModFlags : unsigned {
    None = 0u,
    CapsLock = 1u << 0,
    Shift = 1u << 1,
    Control = 1u << 2,
    Alt = 1u << 3,
    Super = 1u << 4,
    NumLock = 1u << 5,
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
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Num0,
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

/// List of all known mouse buttons
enum struct MouseButton {
    None = -1,
    Left = 0,
    Right = 1,
    Other = 2,
    LAST = Other
};

/// Utility class that captures all input and stores it for easy access during a
/// frame. An object of this class is accessible via the application and can be
/// useful if listening to input events is too tedious
class BLOOM_API Input {
public:
    /// \Returns the current mouse position
    vml::float2 mousePosition() const { return _mousePosition; }

    /// \Returns the offset between the mouse position this frame and last frame
    vml::float2 mouseOffset() const { return _mouseOffset; }

    /// \Returns the current scroll offset
    vml::float2 scrollOffset() const { return _scrollOffset; }

    /// \Returns `true` if \p button is currently pressed
    bool mouseDown(MouseButton button) const {
        return _mouseButtons[(std::size_t)button];
    }

    /// \Returns `true` if \p key is currently pressed
    bool keyDown(Key key) const { return _keys[(std::size_t)key]; }

    /// \Returns the number of registered repeats of \p key while it is held
    /// down
    int keyDownRepeatCount(Key key) const { return _keys[(std::size_t)key]; }

    /// \Returns a bitfield of the currently pressed modifier keys
    ModFlags modFlags() const { return _modFlags; }

private:
    friend class Application;
    friend class Window;

    /// Private setter API used by the `Window` and `Application` classes
    /// @{
    /// Resets mouse and scroll offset to zero
    void endFrame();
    void setKey(Key key, bool down);
    void setMouseButton(MouseButton button, bool down);
    void setMousePosition(vml::float2 position);
    void setScrollOffset(vml::float2 offset);
    /// @}

    ModFlags _modFlags = ModFlags::None;
    vml::float2 _mousePosition = 0;
    vml::float2 _mouseOffset = 0;
    vml::float2 _scrollOffset = 0;
    std::array<int, EnumCount<Key>> _keys{};
    std::array<int, EnumCount<MouseButton>> _mouseButtons{};
};

} // namespace bloom

#endif // BLOOM_APPLICATION_INPUT_H

#include "Bloom/Application/Input.h"

#include <GLFW/glfw3.h>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Application/InputInternal.h"

using namespace bloom;

ModFlags bloom::modFlagsFromGLFW(int glfwFlags) {
    ModFlags result = ModFlags::None;
    if (glfwFlags & GLFW_MOD_SHIFT) {
        result |= ModFlags::Shift;
    }
    if (glfwFlags & GLFW_MOD_CONTROL) {
        result |= ModFlags::Control;
    }
    if (glfwFlags & GLFW_MOD_ALT) {
        result |= ModFlags::Alt;
    }
    if (glfwFlags & GLFW_MOD_SUPER) {
        result |= ModFlags::Super;
    }
    if (glfwFlags & GLFW_MOD_CAPS_LOCK) {
        result |= ModFlags::CapsLock;
    }
    if (glfwFlags & GLFW_MOD_NUM_LOCK) {
        result |= ModFlags::NumLock;
    }
    return result;
}

static std::array<Key, GLFW_KEY_LAST + 1> glfwKeyTranslationTable = [] {
    std::array<Key, GLFW_KEY_LAST + 1> result{};
    result[GLFW_KEY_0] = Key::Num0;
    result[GLFW_KEY_1] = Key::Num1;
    result[GLFW_KEY_2] = Key::Num2;
    result[GLFW_KEY_3] = Key::Num3;
    result[GLFW_KEY_4] = Key::Num4;
    result[GLFW_KEY_5] = Key::Num5;
    result[GLFW_KEY_6] = Key::Num6;
    result[GLFW_KEY_7] = Key::Num7;
    result[GLFW_KEY_8] = Key::Num8;
    result[GLFW_KEY_9] = Key::Num9;
    result[GLFW_KEY_A] = Key::A;
    result[GLFW_KEY_B] = Key::B;
    result[GLFW_KEY_C] = Key::C;
    result[GLFW_KEY_D] = Key::D;
    result[GLFW_KEY_E] = Key::E;
    result[GLFW_KEY_F] = Key::F;
    result[GLFW_KEY_G] = Key::G;
    result[GLFW_KEY_H] = Key::H;
    result[GLFW_KEY_I] = Key::I;
    result[GLFW_KEY_J] = Key::J;
    result[GLFW_KEY_K] = Key::K;
    result[GLFW_KEY_L] = Key::L;
    result[GLFW_KEY_M] = Key::M;
    result[GLFW_KEY_N] = Key::N;
    result[GLFW_KEY_O] = Key::O;
    result[GLFW_KEY_P] = Key::P;
    result[GLFW_KEY_Q] = Key::Q;
    result[GLFW_KEY_R] = Key::R;
    result[GLFW_KEY_S] = Key::S;
    result[GLFW_KEY_T] = Key::T;
    result[GLFW_KEY_U] = Key::U;
    result[GLFW_KEY_V] = Key::V;
    result[GLFW_KEY_W] = Key::W;
    result[GLFW_KEY_X] = Key::X;
    result[GLFW_KEY_Y] = Key::Y;
    result[GLFW_KEY_Z] = Key::Z;
    result[GLFW_KEY_APOSTROPHE] = Key::Apostrophe;
    result[GLFW_KEY_BACKSLASH] = Key::Backslash;
    result[GLFW_KEY_COMMA] = Key::Comma;
    result[GLFW_KEY_EQUAL] = Key::Equal;
    result[GLFW_KEY_GRAVE_ACCENT] = Key::GraveAccent;
    result[GLFW_KEY_LEFT_BRACKET] = Key::LeftBracket;
    result[GLFW_KEY_MINUS] = Key::Minus;
    result[GLFW_KEY_PERIOD] = Key::Period;
    result[GLFW_KEY_RIGHT_BRACKET] = Key::RightBracket;
    result[GLFW_KEY_SEMICOLON] = Key::Semicolon;
    result[GLFW_KEY_SLASH] = Key::Slash;
    result[GLFW_KEY_WORLD_1] = Key::None;
    result[GLFW_KEY_BACKSPACE] = Key::Backspace;
    result[GLFW_KEY_CAPS_LOCK] = Key::CapsLock;
    result[GLFW_KEY_DELETE] = Key::Delete;
    result[GLFW_KEY_DOWN] = Key::DownArrow;
    result[GLFW_KEY_END] = Key::End;
    result[GLFW_KEY_ENTER] = Key::Enter;
    result[GLFW_KEY_ESCAPE] = Key::Escape;
    result[GLFW_KEY_F1] = Key::F1;
    result[GLFW_KEY_F2] = Key::F2;
    result[GLFW_KEY_F3] = Key::F3;
    result[GLFW_KEY_F4] = Key::F4;
    result[GLFW_KEY_F5] = Key::F5;
    result[GLFW_KEY_F6] = Key::F6;
    result[GLFW_KEY_F7] = Key::F7;
    result[GLFW_KEY_F8] = Key::F8;
    result[GLFW_KEY_F9] = Key::F9;
    result[GLFW_KEY_F10] = Key::F10;
    result[GLFW_KEY_F11] = Key::F11;
    result[GLFW_KEY_F12] = Key::F12;
    result[GLFW_KEY_PRINT_SCREEN] = Key::PrintScreen;
    result[GLFW_KEY_F14] = Key::None;
    result[GLFW_KEY_F15] = Key::None;
    result[GLFW_KEY_F16] = Key::None;
    result[GLFW_KEY_F17] = Key::None;
    result[GLFW_KEY_F18] = Key::None;
    result[GLFW_KEY_F19] = Key::None;
    result[GLFW_KEY_F20] = Key::None;
    result[GLFW_KEY_HOME] = Key::Home;
    result[GLFW_KEY_INSERT] = Key::Insert;
    result[GLFW_KEY_LEFT] = Key::LeftArrow;
    result[GLFW_KEY_LEFT_ALT] = Key::LeftAlt;
    result[GLFW_KEY_LEFT_CONTROL] = Key::LeftCtrl;
    result[GLFW_KEY_LEFT_SHIFT] = Key::LeftShift;
    result[GLFW_KEY_LEFT_SUPER] = Key::LeftSuper;
    result[GLFW_KEY_MENU] = Key::Menu;
    result[GLFW_KEY_NUM_LOCK] = Key::NumLock;
    result[GLFW_KEY_PAGE_DOWN] = Key::PageDown;
    result[GLFW_KEY_PAGE_UP] = Key::PageUp;
    result[GLFW_KEY_RIGHT] = Key::RightArrow;
    result[GLFW_KEY_RIGHT_ALT] = Key::RightAlt;
    result[GLFW_KEY_RIGHT_CONTROL] = Key::RightCtrl;
    result[GLFW_KEY_RIGHT_SHIFT] = Key::RightShift;
    result[GLFW_KEY_RIGHT_SUPER] = Key::RightSuper;
    result[GLFW_KEY_SPACE] = Key::Space;
    result[GLFW_KEY_TAB] = Key::Tab;
    result[GLFW_KEY_UP] = Key::UpArrow;
    result[GLFW_KEY_KP_0] = Key::Keypad0;
    result[GLFW_KEY_KP_1] = Key::Keypad1;
    result[GLFW_KEY_KP_2] = Key::Keypad2;
    result[GLFW_KEY_KP_3] = Key::Keypad3;
    result[GLFW_KEY_KP_4] = Key::Keypad4;
    result[GLFW_KEY_KP_5] = Key::Keypad5;
    result[GLFW_KEY_KP_6] = Key::Keypad6;
    result[GLFW_KEY_KP_7] = Key::Keypad7;
    result[GLFW_KEY_KP_8] = Key::Keypad8;
    result[GLFW_KEY_KP_9] = Key::Keypad9;
    result[GLFW_KEY_KP_ADD] = Key::KeypadAdd;
    result[GLFW_KEY_KP_DECIMAL] = Key::KeypadDecimal;
    result[GLFW_KEY_KP_DIVIDE] = Key::KeypadDivide;
    result[GLFW_KEY_KP_ENTER] = Key::KeypadEnter;
    result[GLFW_KEY_KP_EQUAL] = Key::KeypadEqual;
    result[GLFW_KEY_KP_MULTIPLY] = Key::KeypadMultiply;
    result[GLFW_KEY_KP_SUBTRACT] = Key::KeypadSubtract;

    return result;
}();

Key bloom::keyFromGLFW(int glfwCode) {
    if (glfwCode == GLFW_KEY_UNKNOWN) {
        return Key::None;
    }
    return glfwKeyTranslationTable[glfwCode];
}

static std::array<MouseButton, 3> glfwMouseButtonTranslationTable = [] {
    std::array<MouseButton, 3> result{};

    result[GLFW_MOUSE_BUTTON_LEFT] = MouseButton::Left;
    result[GLFW_MOUSE_BUTTON_RIGHT] = MouseButton::Right;
    result[GLFW_MOUSE_BUTTON_MIDDLE] = MouseButton::Other;

    return result;
}();

MouseButton bloom::mouseButtonFromGLFW(int glfwCode) {
    return glfwMouseButtonTranslationTable[glfwCode];
}

void Input::endFrame() {
    _mouseOffset = { 0.0, 0.0 };
    _scrollOffset = { 0.0, 0.0 };
}

void Input::setKey(Key key, bool down) { _keys[(std::size_t)key] = down; }

void Input::setMouseButton(MouseButton button, bool down) {
    _mouseButtons[(std::size_t)button] = down;
}

void Input::setMousePosition(mtl::float2 position) {
    auto lastPos = _mousePosition;
    _mousePosition = position;
    _mouseOffset = position - lastPos;
}

void Input::setScrollOffset(mtl::float2 offset) { _scrollOffset = offset; }

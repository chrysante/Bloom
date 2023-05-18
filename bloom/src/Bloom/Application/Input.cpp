#include "Bloom/Application/Input.hpp"

#include <GLFW/glfw3.h>

#include "Bloom/Application/InputEvent.hpp"

using namespace bloom;

ModFlags bloom::modFlagsFromGLFW(int glfwFlags) {
    ModFlags result = ModFlags::none;

    if (glfwFlags & GLFW_MOD_SHIFT) {
        result |= ModFlags::shift;
    }
    if (glfwFlags & GLFW_MOD_CONTROL) {
        result |= ModFlags::control;
    }
    if (glfwFlags & GLFW_MOD_ALT) {
        result |= ModFlags::alt;
    }
    if (glfwFlags & GLFW_MOD_SUPER) {
        result |= ModFlags::super;
    }
    if (glfwFlags & GLFW_MOD_CAPS_LOCK) {
        result |= ModFlags::capsLock;
    }
    if (glfwFlags & GLFW_MOD_NUM_LOCK) {
        result |= ModFlags::numLock;
    }

    return result;
}

static std::array<Key, GLFW_KEY_LAST + 1> glfwKeyTranslationTable = [] {
    std::array<Key, GLFW_KEY_LAST + 1> result{};
    result[GLFW_KEY_0] = Key::_0;
    result[GLFW_KEY_1] = Key::_1;
    result[GLFW_KEY_2] = Key::_2;
    result[GLFW_KEY_3] = Key::_3;
    result[GLFW_KEY_4] = Key::_4;
    result[GLFW_KEY_5] = Key::_5;
    result[GLFW_KEY_6] = Key::_6;
    result[GLFW_KEY_7] = Key::_7;
    result[GLFW_KEY_8] = Key::_8;
    result[GLFW_KEY_9] = Key::_9;
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

    result[GLFW_KEY_APOSTROPHE]    = Key::apostrophe;
    result[GLFW_KEY_BACKSLASH]     = Key::backslash;
    result[GLFW_KEY_COMMA]         = Key::comma;
    result[GLFW_KEY_EQUAL]         = Key::equal;
    result[GLFW_KEY_GRAVE_ACCENT]  = Key::graveAccent;
    result[GLFW_KEY_LEFT_BRACKET]  = Key::leftBracket;
    result[GLFW_KEY_MINUS]         = Key::minus;
    result[GLFW_KEY_PERIOD]        = Key::period;
    result[GLFW_KEY_RIGHT_BRACKET] = Key::rightBracket;
    result[GLFW_KEY_SEMICOLON]     = Key::semicolon;
    result[GLFW_KEY_SLASH]         = Key::slash;
    result[GLFW_KEY_WORLD_1]       = Key::_1;

    result[GLFW_KEY_BACKSPACE]     = Key::backspace;
    result[GLFW_KEY_CAPS_LOCK]     = Key::capsLock;
    result[GLFW_KEY_DELETE]        = Key::delete_;
    result[GLFW_KEY_DOWN]          = Key::downArrow;
    result[GLFW_KEY_END]           = Key::end;
    result[GLFW_KEY_ENTER]         = Key::enter;
    result[GLFW_KEY_ESCAPE]        = Key::escape;
    result[GLFW_KEY_F1]            = Key::F1;
    result[GLFW_KEY_F2]            = Key::F2;
    result[GLFW_KEY_F3]            = Key::F3;
    result[GLFW_KEY_F4]            = Key::F4;
    result[GLFW_KEY_F5]            = Key::F5;
    result[GLFW_KEY_F6]            = Key::F6;
    result[GLFW_KEY_F7]            = Key::F7;
    result[GLFW_KEY_F8]            = Key::F8;
    result[GLFW_KEY_F9]            = Key::F9;
    result[GLFW_KEY_F10]           = Key::F10;
    result[GLFW_KEY_F11]           = Key::F11;
    result[GLFW_KEY_F12]           = Key::F12;
    result[GLFW_KEY_PRINT_SCREEN]  = Key::printScreen;
    result[GLFW_KEY_F14]           = Key::none;
    result[GLFW_KEY_F15]           = Key::none;
    result[GLFW_KEY_F16]           = Key::none;
    result[GLFW_KEY_F17]           = Key::none;
    result[GLFW_KEY_F18]           = Key::none;
    result[GLFW_KEY_F19]           = Key::none;
    result[GLFW_KEY_F20]           = Key::none;
    result[GLFW_KEY_HOME]          = Key::home;
    result[GLFW_KEY_INSERT]        = Key::insert;
    result[GLFW_KEY_LEFT]          = Key::leftArrow;
    result[GLFW_KEY_LEFT_ALT]      = Key::leftAlt;
    result[GLFW_KEY_LEFT_CONTROL]  = Key::leftCtrl;
    result[GLFW_KEY_LEFT_SHIFT]    = Key::leftShift;
    result[GLFW_KEY_LEFT_SUPER]    = Key::leftSuper;
    result[GLFW_KEY_MENU]          = Key::menu;
    result[GLFW_KEY_NUM_LOCK]      = Key::numLock;
    result[GLFW_KEY_PAGE_DOWN]     = Key::pageDown;
    result[GLFW_KEY_PAGE_UP]       = Key::pageUp;
    result[GLFW_KEY_RIGHT]         = Key::rightArrow;
    result[GLFW_KEY_RIGHT_ALT]     = Key::rightAlt;
    result[GLFW_KEY_RIGHT_CONTROL] = Key::rightCtrl;
    result[GLFW_KEY_RIGHT_SHIFT]   = Key::rightShift;
    result[GLFW_KEY_RIGHT_SUPER]   = Key::rightSuper;
    result[GLFW_KEY_SPACE]         = Key::space;
    result[GLFW_KEY_TAB]           = Key::tab;
    result[GLFW_KEY_UP]            = Key::upArrow;

    result[GLFW_KEY_KP_0]        = Key::keypad0;
    result[GLFW_KEY_KP_1]        = Key::keypad1;
    result[GLFW_KEY_KP_2]        = Key::keypad2;
    result[GLFW_KEY_KP_3]        = Key::keypad3;
    result[GLFW_KEY_KP_4]        = Key::keypad4;
    result[GLFW_KEY_KP_5]        = Key::keypad5;
    result[GLFW_KEY_KP_6]        = Key::keypad6;
    result[GLFW_KEY_KP_7]        = Key::keypad7;
    result[GLFW_KEY_KP_8]        = Key::keypad8;
    result[GLFW_KEY_KP_9]        = Key::keypad9;
    result[GLFW_KEY_KP_ADD]      = Key::keypadAdd;
    result[GLFW_KEY_KP_DECIMAL]  = Key::keypadDecimal;
    result[GLFW_KEY_KP_DIVIDE]   = Key::keypadDivide;
    result[GLFW_KEY_KP_ENTER]    = Key::keypadEnter;
    result[GLFW_KEY_KP_EQUAL]    = Key::keypadEqual;
    result[GLFW_KEY_KP_MULTIPLY] = Key::keypadMultiply;
    result[GLFW_KEY_KP_SUBTRACT] = Key::keypadSubtract;

    return result;
}();

Key bloom::keyFromGLFW(int glfwCode) {
    if (glfwCode == GLFW_KEY_UNKNOWN) {
        return Key::none;
    }
    return glfwKeyTranslationTable[glfwCode];
}

static std::array<MouseButton, 3> glfwMouseButtonTranslationTable = [] {
    std::array<MouseButton, 3> result{};

    result[GLFW_MOUSE_BUTTON_LEFT]   = MouseButton::left;
    result[GLFW_MOUSE_BUTTON_RIGHT]  = MouseButton::right;
    result[GLFW_MOUSE_BUTTON_MIDDLE] = MouseButton::other;

    return result;
}();

MouseButton bloom::mouseButtonFromGLFW(int glfwCode) {
    return glfwMouseButtonTranslationTable[glfwCode];
}

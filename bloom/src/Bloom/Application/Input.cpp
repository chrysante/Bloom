#include "Bloom/Application/Input.h"

#include <GLFW/glfw3.h>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Application/InputInternal.h"
#include "Bloom/Application/Window.h"

using namespace bloom;
using namespace vml::short_types;

vml::float2 Input::mousePositionInWindow(Window const& window) const {
    return mousePosition() - window.position();
}

struct Input::Impl {
    static void updateMousePos(Input& input, double2 position) {
        auto lastPos = input._mousePosition;
        input._mousePosition = position;
        input._mouseOffset = position - lastPos;
    }

    static void updateCommon(Input& input, auto const& event) {
        if constexpr (requires { event.modFlags; }) {
            input._modFlags = event.modFlags;
        }
    }

    static void update(Input& input, InputEvent const& event) {
        using enum InputEventMask;
        event.dispatch<MouseMoved>([&](MouseEvent const& event) {
            updateCommon(input, event);
            updateMousePos(input, event.locationOnScreen);
        });
        event.dispatch<MouseDragged>([&](MouseEvent const& event) {
            updateCommon(input, event);
            updateMousePos(input, event.locationOnScreen);
        });
        event.dispatch<MouseDown>([&](MouseDownEvent const& event) {
            updateCommon(input, event);
            input._mouseButtonDown[(size_t)event.button] = true;
        });
        event.dispatch<MouseUp>([&](MouseUpEvent const& event) {
            updateCommon(input, event);
            input._mouseButtonDown[(size_t)event.button] = false;
        });
        event.dispatch<ScrollWheel>([&](ScrollEvent const& event) {
            updateCommon(input, event);
            input._scrollOffset = event.offset;
        });
        event.dispatch<KeyDown>([&](KeyEvent const& event) {
            updateCommon(input, event);
            input._keyDown[(size_t)event.key] = true;
        });
        event.dispatch<KeyUp>([&](KeyEvent const& event) {
            updateCommon(input, event);
            input._keyDown[(size_t)event.key] = false;
        });
    }

    static void endFrame(Input& input) {
        input._mouseOffset = { 0.0, 0.0 };
        input._scrollOffset = { 0.0, 0.0 };
    }
};

using Impl = Input::Impl;

void bloom::inputUpdate(Input& input, InputEvent const& event) {
    Impl::update(input, event);
}

void bloom::inputEndFrame(Input& input) { Impl::endFrame(input); }

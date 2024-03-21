#include "Bloom/Application/InputEvent.h"

#include <ostream>

#include <GLFW/glfw3.h>

#include "Bloom/Application/InputInternal.h"

using namespace bloom;

InputEvent bloom::makeInputEventFromGLFWMouseButton(Input const& input,
                                                    int buttonCode, int action,
                                                    [[maybe_unused]] int mods) {
    auto const [type, button] = [&] {
        switch (buttonCode) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return std::pair{ action == GLFW_PRESS ?
                                  InputEventMask::LeftMouseDown :
                                  InputEventMask::LeftMouseUp,
                              MouseButton::Left };
        case GLFW_MOUSE_BUTTON_RIGHT:
            return std::pair{ action == GLFW_PRESS ?
                                  InputEventMask::RightMouseDown :
                                  InputEventMask::RightMouseUp,
                              MouseButton::Right };
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return std::pair{ action == GLFW_PRESS ?
                                  InputEventMask::OtherMouseDown :
                                  InputEventMask::OtherMouseUp,
                              MouseButton::Other };

        default:
            return std::pair{ InputEventMask::None, MouseButton::None };
        }
    }();

    MouseEvent event;
    event.modifierFlags = input.modFlags();
    event.locationInWindow = input.mousePosition();

    switch (action) {
    case GLFW_PRESS: {
        MouseDownEvent downEvent{
            event, .button = button,
            .clickCount = 1 // TODO: get correct click count
        };
        return InputEvent(type, downEvent);
    }

    case GLFW_RELEASE: {
        MouseUpEvent upEvent{ event, .button = button };
        return InputEvent(type, upEvent);
    }
    default:
        // TODO: Check if these are being genereated
        BL_UNIMPLEMENTED();
    }
}

InputEvent bloom::makeInputEventFromGLFWCursorPos(Input const& input, double,
                                                  double) {
    MouseMoveEvent event;
    event.modifierFlags = input.modFlags();
    event.locationInWindow = input.mousePosition();
    event.offset = input.mouseOffset();
    MouseDragEvent dragEvent{ event, .button = MouseButton::None };
    if (input.mouseDown(MouseButton::Left)) {
        dragEvent.button = MouseButton::Left;
        return InputEvent(InputEventMask::LeftMouseDragged, dragEvent);
    }
    else if (input.mouseDown(MouseButton::Right)) {
        dragEvent.button = MouseButton::Right;
        return InputEvent(InputEventMask::RightMouseDragged, dragEvent);
    }
    else if (input.mouseDown(MouseButton::Other)) {
        dragEvent.button = MouseButton::Other;
        return InputEvent(InputEventMask::OtherMouseDragged, dragEvent);
    }
    else {
        return InputEvent(InputEventMask::MouseMoved, event);
    }
}

InputEvent bloom::makeInputEventFromGLFWScroll(Input const& input,
                                               double xoffset, double yoffset) {
    ScrollEvent event;
    event.modifierFlags = input.modFlags();
    event.locationInWindow = input.mousePosition();
    event.offset = { xoffset, yoffset };
    return InputEvent(InputEventMask::ScrollWheel, event);
}

InputEvent bloom::makeInputEventFromGLFWKey(Input const& input, int keyCode,
                                            [[maybe_unused]] int scancode,
                                            int action,
                                            [[maybe_unused]] int mods) {
    Key key = keyFromGLFW(keyCode);
    KeyEvent event;
    event.modifierFlags = input.modFlags();
    event.repeat = input.keyDownRepeatCount(key); // TODO: Fix this
    event.key = key;
    switch (action) {
    case GLFW_PRESS:
    case GLFW_REPEAT:
        return InputEvent(InputEventMask::KeyDown, event);

    case GLFW_RELEASE: {
        return InputEvent(InputEventMask::KeyUp, event);
    }
    default:
        // TODO: Check if these are being genereated
        BL_UNIMPLEMENTED();
    }
}

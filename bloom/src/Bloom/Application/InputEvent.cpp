#include "Bloom/Application/InputEvent.h"

#include <ostream>

#include <GLFW/glfw3.h>

using namespace bloom;

std::string_view bloom::toString(InputEventType e) {
    return std::array{ "mouseDown",       "rightMouseDown",
                       "otherMouseDown",  "mouseUp",
                       "rightMouseUp",    "otherMouseUp",
                       "mouseMoved",      "mouseDragged",
                       "rightMouseMoved", "rightMouseDragged",
                       "otherMouseMoved", "otherMouseDragged",
                       "scrollWheel",     "magnify" }[(unsigned)e];
}

std::ostream& operator<<(std::ostream& str, InputEventType e) {
    return str << toString(e);
}

InputEvent bloom::inputEventFromGLFWMouseButton(Input const& input,
                                                int buttonCode, int action,
                                                [[maybe_unused]] int mods) {
    auto const [type, button] = [&] {
        switch (buttonCode) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return std::pair{ action == GLFW_PRESS ?
                                  InputEventType::leftMouseDown :
                                  InputEventType::leftMouseUp,
                              MouseButton::Left };
        case GLFW_MOUSE_BUTTON_RIGHT:
            return std::pair{ action == GLFW_PRESS ?
                                  InputEventType::rightMouseDown :
                                  InputEventType::rightMouseUp,
                              MouseButton::Right };
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return std::pair{ action == GLFW_PRESS ?
                                  InputEventType::otherMouseDown :
                                  InputEventType::otherMouseUp,
                              MouseButton::Other };

        default:
            return std::pair{ InputEventType::none, MouseButton::None };
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
        BL_DEBUGFAIL("Are these even generated?");
    }
}

InputEvent bloom::inputEventFromGLFWCursorPos(Input const& input, double,
                                              double) {
    MouseMoveEvent event;
    event.modifierFlags = input.modFlags();
    event.locationInWindow = input.mousePosition();
    event.offset = input.mouseOffset();
    MouseDragEvent dragEvent{ event, .button = MouseButton::None };
    if (input.mouseDown(MouseButton::Left)) {
        dragEvent.button = MouseButton::Left;
        return InputEvent(InputEventType::leftMouseDragged, dragEvent);
    }
    else if (input.mouseDown(MouseButton::Right)) {
        dragEvent.button = MouseButton::Right;
        return InputEvent(InputEventType::rightMouseDragged, dragEvent);
    }
    else if (input.mouseDown(MouseButton::Other)) {
        dragEvent.button = MouseButton::Other;
        return InputEvent(InputEventType::otherMouseDragged, dragEvent);
    }
    else {
        return InputEvent(InputEventType::mouseMoved, event);
    }
}

InputEvent bloom::inputEventFromGLFWScroll(Input const& input, double xoffset,
                                           double yoffset) {
    ScrollEvent event;
    event.modifierFlags = input.modFlags();
    event.locationInWindow = input.mousePosition();
    event.offset = { xoffset, yoffset };
    return InputEvent(InputEventType::scrollWheel, event);
}

InputEvent bloom::inputEventFromGLFWKey(Input const& input, int keyCode,
                                        [[maybe_unused]] int scancode,
                                        int action, [[maybe_unused]] int mods) {
    Key key = keyFromGLFW(keyCode);
    KeyEvent event;
    event.modifierFlags = input.modFlags();
    event.repeat = input.keyDownRepeatCount(key); // TODO: Fix this
    event.key = key;
    switch (action) {
    case GLFW_PRESS:
    case GLFW_REPEAT:
        return InputEvent(InputEventType::keyDown, event);

    case GLFW_RELEASE: {
        return InputEvent(InputEventType::keyUp, event);
    }
    default:
        BL_DEBUGFAIL("Are these even generated?");
    }
}

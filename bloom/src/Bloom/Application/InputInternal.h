#ifndef BLOOM_APPLICATION_INPUTINTERNAL_H
#define BLOOM_APPLICATION_INPUTINTERNAL_H

#include <vml/vml.hpp>

namespace bloom {

class Window;

/// Translates GLFW mouse button codes to ours
MouseButton mouseButtonFromGLFW(int buttonCode);

class InputEvent;

/// Constructor functions
/// @{
InputEvent makeInputEventFromGLFWMouseButton(Input const&, Window const& window,
                                             int button, int action, int mods);
InputEvent makeInputEventFromGLFWCursorPos(Input const&, Window const& window);
InputEvent makeInputEventFromGLFWScroll(Input const&, Window const& window,
                                        double xoffset, double yoffset);
InputEvent makeInputEventFromGLFWKey(Input const&, int key, int scancode,
                                     int action, int mods);
/// @}

} // namespace bloom

#endif // BLOOM_APPLICATION_INPUTINTERNAL_H

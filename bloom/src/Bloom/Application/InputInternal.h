#ifndef BLOOM_APPLICATION_INPUTINTERNAL_H
#define BLOOM_APPLICATION_INPUTINTERNAL_H

namespace bloom {

/// Translates GLFW mouse button codes to ours
MouseButton mouseButtonFromGLFW(int buttonCode);

class InputEvent;

/// Constructor functions
/// @{
InputEvent makeInputEventFromGLFWMouseButton(Input const&, int button,
                                             int action, int mods);
InputEvent makeInputEventFromGLFWCursorPos(Input const&, double xpos,
                                           double ypos);
InputEvent makeInputEventFromGLFWScroll(Input const&, double xoffset,
                                        double yoffset);
InputEvent makeInputEventFromGLFWKey(Input const&, int key, int scancode,
                                     int action, int mods);
/// @}

} // namespace bloom

#endif // BLOOM_APPLICATION_INPUTINTERNAL_H

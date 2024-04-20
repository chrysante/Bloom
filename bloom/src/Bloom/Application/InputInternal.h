#ifndef BLOOM_APPLICATION_INPUTINTERNAL_H
#define BLOOM_APPLICATION_INPUTINTERNAL_H

#include <functional>

#include <vml/vml.hpp>

namespace bloom {

class Input;
class InputEvent;

void inputUpdate(Input& input, InputEvent const& event);

void inputEndFrame(Input& input);

/// Registers \p callback to be invoked with every input event that is sent to
/// the application \Returns an ID that can be used to unregister the callback
[[nodiscard]] int registerGlobalInputCallback(
    std::function<void(InputEvent const&, void* nativeEvent)> handler);

/// Unregisters the callback with ID \p ID
void unregisterGlobalInputCallback(int ID);

} // namespace bloom

#endif // BLOOM_APPLICATION_INPUTINTERNAL_H

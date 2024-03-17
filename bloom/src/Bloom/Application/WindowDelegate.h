#ifndef BLOOM_APPLICATION_WINDOWDELEGATE_H
#define BLOOM_APPLICATION_WINDOWDELEGATE_H

#include "Bloom/Application/Window.h"
#include "Bloom/Core/Base.h"

namespace bloom {

/// Callback class to customize window behaviour
class BLOOM_API WindowDelegate {
public:
    virtual ~WindowDelegate() = default;

    /// FIXME: Should this be called `onAttach()`?
    /// Called after being attached to a window
    virtual void init() {}

    /// TODO: Do we need this?
    virtual void shutdown() {}

    /// Called on every frame
    virtual void frame() {}

    /// \Returns the attached window
    Window& window() { return *wnd; }

private:
    friend class Application;
    Window* wnd;
};

}; // namespace bloom

#endif // BLOOM_APPLICATION_WINDOWDELEGATE_H

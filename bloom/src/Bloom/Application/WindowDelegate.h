#ifndef BLOOM_APPLICATION_WINDOWDELEGATE_H
#define BLOOM_APPLICATION_WINDOWDELEGATE_H

#include "Bloom/Application/Window.h"
#include "Bloom/Core/Base.h"

namespace bloom {

class BLOOM_API WindowDelegate {
public:
    virtual ~WindowDelegate() = default;

    virtual void init() {}
    virtual void shutdown() {}

    virtual void frame() {}

    Window& window() { return *theWindow; }
    Application& application() { return theWindow->application(); }

private:
    friend class Application;
    Window* theWindow;
};

}; // namespace bloom

#endif // BLOOM_APPLICATION_WINDOWDELEGATE_H

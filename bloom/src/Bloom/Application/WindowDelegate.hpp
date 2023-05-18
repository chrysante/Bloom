#pragma once

#include "Bloom/Application/Window.hpp"
#include "Bloom/Core/Base.hpp"

namespace bloom {

class BLOOM_API WindowDelegate {
public:
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

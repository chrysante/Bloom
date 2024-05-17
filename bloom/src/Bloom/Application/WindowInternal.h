#ifndef WindowInternal_h
#define WindowInternal_h

#include <GLFW/glfw3.h>
#include <vml/vml.hpp>

#include "Bloom/Application/Window.h"

namespace bloom {

struct Window::Impl: WindowDescription {
    struct Deleter {
        void operator()(GLFWwindow*) const;
    };

    ~Impl();

    void resizeSwapchain(vml::int2 newSize);

    template <typename... Args, typename... Args2>
    void invokeDelegates(void (WindowDelegate::*callback)(Args...),
                         Args2&&... args) const {
        for (auto& del: delegates) {
            (del.get()->*callback)(std::forward<Args2>(args)...);
        }
    }

    /// Backup positition and size to restore from fullscreen
    vml::int2 backupPosition = 0;
    vml::int2 backupSize = 0;
    vml::float2 contentScaleFactor = 1;
    vml::int2 minSize = 0;
    vml::int2 maxSize = 0;
    bool focused = false;
    bool shallPreventClose = false;
    /// See `setMovable()`
    bool movable = true;
    /// We set this flag when we call `performWindowDragWithEvent` and in
    /// the window position callback and clear it whenever the left mouse
    /// button is released
    bool isMoving = false;
    bool requestedMove = false;
    bool requestedResize = false;
    std::unique_ptr<GLFWwindow, Deleter> glfwWnd;
    void* nativeWindow = nullptr;
    Reference<Swapchain> swapchain;
    std::vector<std::unique_ptr<WindowDelegate>> delegates;
    std::shared_ptr<Window::Impl> _self;
};

} // namespace bloom

#endif /* WindowInternal_h */

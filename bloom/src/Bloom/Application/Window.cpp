#include "Bloom/Application/Window.hpp"

#include <optional>

#include <GLFW/glfw3.h>

#include "Bloom/Core/Debug.hpp"
#include "Bloom/GPU/HardwareDevice.hpp"
#include "Bloom/GPU/Swapchain.hpp"

#define GLFW_WND ((GLFWwindow*)glfwWindowPtr.get())

using namespace mtl::short_types;
using namespace bloom;

/// MARK: - Statics
///
///
void Window::pollEvents() {
    glfwPollEvents();
    //		glfwWaitEvents();
}

void Window::initWindowSystem() {
    int const status = glfwInit();
    if (status != GLFW_TRUE) {
        Logger::Fatal("Failed to initialize GLFW");
        std::terminate();
    }
}

/// MARK: - Initialization
///
///
Window::Window(WindowDescription const& d) {
    desc = WindowDescPrivate{ d };

    desc.fullscreen = false;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* const w = glfwCreateWindow(desc.size.x, desc.size.y,
                                     desc.title.data(), nullptr, nullptr);
    glfwWindowPtr = std::unique_ptr<void, Deleter>(w);

    platformInit();

    glfwSetWindowUserPointer(GLFW_WND, this);

    glfwGetWindowPos(GLFW_WND, &desc.position.x, &desc.position.y);
    glfwGetWindowSize(GLFW_WND, &desc.size.x, &desc.size.y);
    glfwGetWindowContentScale(GLFW_WND, &desc.contentScaleFactor.x,
                              &desc.contentScaleFactor.y);

    setCallbacks();
}

Window::~Window() = default;

void Window::createDefaultSwapchain(HardwareDevice& device,
                                    std::size_t backbufferCount) {
    SwapchainDescription swapchainDesc;
    swapchainDesc.displaySync = true;
    swapchainDesc.size = size() * contentScaleFactor();
    swapchainDesc.backBufferCount = backbufferCount;

    auto swapchain = device.createSwapchain(swapchainDesc);
    setSwapchain(std::move(swapchain));
}

//	void Window::setSwapchain(std::unique_ptr<Swapchain>); // defined in
// Platform/.../CocoaWindow.mm

void Window::setCommandQueue(std::unique_ptr<CommandQueue> queue) {
    theCommandQueue = std::move(queue);
}

/// MARK: Update
///
///
void Window::beginFrame() {}

void Window::endFrame() {
    userInput._mouseOffset = 0;
    userInput._scrollOffset = 0;
}

/// MARK: Callbacks
///
///
void Window::onInput(utl::function<void(InputEvent const&)> f) {
    onInputFn = std::move(f);
}

void Window::onCharInput(utl::function<void(unsigned)> f) {
    onCharInputFn = std::move(f);
}

void Window::onFileDrop(
    utl::function<void(utl::vector<std::filesystem::path> const&)> f) {
    onFileDropFn = std::move(f);
}

void Window::onMove(utl::function<void(mtl::int2 newPosition)> f) {
    onMoveFn = std::move(f);
}

void Window::onResize(utl::function<void(mtl::int2 newSize)> f) {
    onResizeFn = std::move(f);
}

void Window::onFocus(utl::function<void()> f) { onFocusFn = std::move(f); }

void Window::onFocusLoss(utl::function<void()> f) {
    onFocusLossFn = std::move(f);
}

void Window::onClose(utl::function<void()> f) { onCloseFn = std::move(f); }

void Window::onContentScaleChange(
    utl::function<void(mtl::float2 newContentScale)> f) {
    onContentScaleChangeFn = std::move(f);
}

/// MARK: Queries
///
///
bool Window::shouldClose() const {
    return !desc.shallPreventClose && glfwWindowShouldClose(GLFW_WND);
}

/// MARK: Modifiers
///
///
void Window::setTitle(std::string newTitle) {
    desc.title = newTitle;
    glfwSetWindowTitle(GLFW_WND, newTitle.data());
}

void Window::setPosition(mtl::int2 newPosition) {
    glfwSetWindowPos(GLFW_WND, newPosition.x, newPosition.y);
    glfwGetWindowPos(GLFW_WND, &desc.position.x, &desc.position.y);
}

void Window::setSize(mtl::int2 newSize) {
    newSize.x = std::max(newSize.x, 0);
    newSize.y = std::max(newSize.y, 0);
    glfwSetWindowSize(GLFW_WND, newSize.x, newSize.y);
    glfwGetWindowSize(GLFW_WND, &desc.size.x, &desc.size.y);
}

void Window::setMinSize(mtl::int2 size) {
    size.x = std::max(size.x, 0);
    size.y = std::max(size.y, 0);
    desc.minSize = size;
    glfwSetWindowSizeLimits(GLFW_WND,
                            desc.minSize.x > 0 ? desc.minSize.x :
                                                 GLFW_DONT_CARE,
                            desc.minSize.y > 0 ? desc.minSize.y :
                                                 GLFW_DONT_CARE,
                            desc.maxSize.x > 0 ? desc.maxSize.x :
                                                 GLFW_DONT_CARE,
                            desc.maxSize.y > 0 ? desc.maxSize.y :
                                                 GLFW_DONT_CARE);
}

void Window::setMaxSize(mtl::int2 size) {
    size.x = std::max(size.x, 0);
    size.y = std::max(size.y, 0);
    desc.maxSize = size;
    glfwSetWindowSizeLimits(GLFW_WND,
                            desc.minSize.x > 0 ? desc.minSize.x :
                                                 GLFW_DONT_CARE,
                            desc.minSize.y > 0 ? desc.minSize.y :
                                                 GLFW_DONT_CARE,
                            desc.maxSize.x > 0 ? desc.maxSize.x :
                                                 GLFW_DONT_CARE,
                            desc.maxSize.y > 0 ? desc.maxSize.y :
                                                 GLFW_DONT_CARE);
}

void Window::setFocused() { glfwFocusWindow(GLFW_WND); }

void Window::show() { glfwShowWindow(GLFW_WND); }

void Window::hide() { glfwHideWindow(GLFW_WND); }

void Window::maximize() { glfwMaximizeWindow(GLFW_WND); }

void Window::minimize() { glfwIconifyWindow(GLFW_WND); }

void Window::restore() { glfwRestoreWindow(GLFW_WND); }

void Window::makeFullscreen() {
    if (isFullscreen()) {
        return;
    }
    auto* const monitor = glfwGetPrimaryMonitor();
    auto* const vidMode = glfwGetVideoMode(monitor);
    int2 const monitorPos = 0;
    int2 const monitorSize = { vidMode->width, vidMode->height };

    desc.backupSize = desc.size;
    desc.size = monitorSize;
    desc.backupPosition = desc.position;
    desc.position = monitorPos;
    desc.fullscreen = true;

    glfwSetWindowMonitor(GLFW_WND, monitor, monitorPos.x, monitorPos.y,
                         monitorSize.x, monitorSize.y, GLFW_DONT_CARE);
}

void Window::toggleFullscreen() {
    if (isFullscreen()) {
        makeWindowed();
    }
    else {
        makeFullscreen();
    }
}

void Window::makeWindowed() {
    if (isWindowed()) {
        return;
    }
    desc.size = desc.backupSize;
    desc.position = desc.backupPosition;
    desc.fullscreen = false;
    glfwSetWindowMonitor(GLFW_WND, nullptr, desc.position.x, desc.position.y,
                         desc.size.x, desc.size.y, GLFW_DONT_CARE);
}

void Window::close() { glfwSetWindowShouldClose(GLFW_WND, true); }

void Window::stopClose() { glfwSetWindowShouldClose(GLFW_WND, false); }

void Window::requestAttention() { glfwRequestWindowAttention(GLFW_WND); }

/// MARK: Private
///
///
#if !defined(BLOOM_PLATFORM_APPLE)
void Window::platformInit() {}
#endif

void Window::setCallbacks() {
    glfwSetMouseButtonCallback(GLFW_WND, [](GLFWwindow* w, int button,
                                            int action, int mods) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);

        window.userInput
            ._mouseButtons[(std::size_t)mouseButtonFromGLFW(button)] = action;
        window.userInput._modFlags = modFlagsFromGLFW(mods);

        if (window.onInputFn) {
            auto const event = inputEventFromGLFWMouseButton(window.userInput,
                                                             button, action,
                                                             mods);
            window.onInputFn(event);
        }
    });

    glfwSetCursorPosCallback(GLFW_WND,
                             [](GLFWwindow* w, double xpos, double ypos) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);

        float2 const currentPos = { xpos, ypos };
        float2 const lastPos = window.userInput._mousePosition;

        window.userInput._mousePosition = currentPos;
        window.userInput._mouseOffset = currentPos - lastPos;

        if (window.onInputFn) {
            window.onInputFn(
                inputEventFromGLFWCursorPos(window.userInput, xpos, ypos));
        }
    });

    glfwSetCursorEnterCallback(GLFW_WND,
                               [](GLFWwindow* w, [[maybe_unused]] int entered) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
    });

    glfwSetScrollCallback(GLFW_WND,
                          [](GLFWwindow* w, double xoffset, double yoffset) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);

        window.userInput._scrollOffset = { xoffset, yoffset };

        if (window.onInputFn) {
            window.onInputFn(
                inputEventFromGLFWScroll(window.userInput, xoffset, yoffset));
        }
    });

    glfwSetKeyCallback(GLFW_WND, [](GLFWwindow* w, int key, int scancode,
                                    int action, int mods) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);

        int& repeatCount =
            window.userInput._keys[(std::size_t)keyFromGLFW(key)];
        switch (action) {
        case GLFW_RELEASE:
            repeatCount = 0;
            break;
        case GLFW_PRESS:
            repeatCount = 1;
            break;
        case GLFW_REPEAT:
            ++repeatCount;
            break;

        default:
            break;
        }
        window.userInput._modFlags = modFlagsFromGLFW(mods);

        if (window.onInputFn) {
            window.onInputFn(inputEventFromGLFWKey(window.userInput, key,
                                                   scancode, action, mods));
        }
    });

    glfwSetCharCallback(GLFW_WND, [](GLFWwindow* w, unsigned int codepoint) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        if (window.onCharInputFn) {
            window.onCharInputFn(codepoint);
        }
    });

    glfwSetDropCallback(GLFW_WND,
                        [](GLFWwindow* w, int pathCount, char const* paths[]) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        utl::small_vector<std::filesystem::path> pathVec;
        pathVec.reserve(pathCount);
        for (int i = 0; i < pathCount; ++i) {
            pathVec.push_back(std::filesystem::path(paths[i]));
        }
        if (window.onFileDropFn) {
            window.onFileDropFn(pathVec);
        }
    });

    glfwSetWindowPosCallback(GLFW_WND, [](GLFWwindow* w, int posx, int posy) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        int2 const pos = { posx, posy };
        window.desc.position = pos;
        if (window.onMovePrivateFn) {
            window.onMovePrivateFn(pos);
        }
        if (window.onMoveFn) {
            window.onMoveFn(pos);
        }
    });

    glfwSetWindowSizeCallback(GLFW_WND,
                              [](GLFWwindow* w, int sizex, int sizey) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        int2 const newSize = { sizex, sizey };
        window.desc.size = newSize;
        window.resizeSwapchain(newSize);
        if (window.onResizePrivateFn) {
            window.onResizePrivateFn(newSize);
        }
        if (window.onResizeFn) {
            window.onResizeFn(newSize);
        }
    });

    glfwSetWindowFocusCallback(GLFW_WND, [](GLFWwindow* w, int focus) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);

        if (focus == GLFW_TRUE) {
            window.desc.focused = true;
            if (window.onFocusFn) {
                window.onFocusFn();
            }
        }
        else {
            window.desc.focused = false;
            if (window.onFocusLossFn) {
                window.onFocusLossFn();
            }
        }
    });

    glfwSetWindowCloseCallback(GLFW_WND, [](GLFWwindow* w) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        window.desc.shallPreventClose = false;
        if (window.onCloseFn) {
            window.onCloseFn();
        }
    });

    glfwSetWindowContentScaleCallback(GLFW_WND,
                                      [](GLFWwindow* w, float x, float y) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        float2 const contentScale = { x, y };
        window.desc.contentScaleFactor = contentScale;
        if (window.onContentScaleChangeFn) {
            window.onContentScaleChangeFn(contentScale);
        }
    });
}

static std::optional<int> calcNewSize(int currentSize, int newSize,
                                      int padding) {
    if (newSize <= currentSize && newSize > currentSize - padding) {
        return std::nullopt;
    }
    if (newSize > currentSize) {
        if (padding > 1) {
            while (newSize > currentSize) {
                currentSize += padding;
            }
            return currentSize;
        }
        else {
            return newSize;
        }
    }
    else {
        if (padding > 1) {
            while (newSize <= currentSize - padding) {
                currentSize -= padding;
            }
            return currentSize;
        }
        else {
            return newSize;
        }
    }
}

void Window::resizeSwapchain(mtl::int2 newSize) {
    if (!desc.autoResizeSwapchain) {
        return;
    }
    if (!theSwapchain) {
        return;
    }

    bool const IGNORE_RESIZE_PADDING = true;
    if (IGNORE_RESIZE_PADDING) {
        theSwapchain->resize(newSize * contentScaleFactor());
        return;
    }

    int2 const currentSize = theSwapchain->description().size;

    auto newSizeX =
        calcNewSize(currentSize.x, newSize.x, desc.swapchainResizePadding.x);
    auto newSizeY =
        calcNewSize(currentSize.y, newSize.y, desc.swapchainResizePadding.y);

    if (newSizeX || newSizeY) {
        auto const newSizePadded = int2(newSizeX.value_or(currentSize.x),
                                        newSizeY.value_or(currentSize.y));
        assert(newSizePadded.x >= newSize.x);
        assert(newSizePadded.y >= newSize.y);
        theSwapchain->resize(newSizePadded * contentScaleFactor());
    }
}

void Window::Deleter::operator()(void* ptr) const {
    glfwDestroyWindow((GLFWwindow*)ptr);
}

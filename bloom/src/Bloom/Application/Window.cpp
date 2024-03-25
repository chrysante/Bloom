#include "Bloom/Application/Window.h"

#include <optional>

#include <GLFW/glfw3.h>

#include "Bloom/Application/InputInternal.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/Swapchain.h"

#define GLFW_WND ((GLFWwindow*)windowPtr.get())

using namespace vml::short_types;
using namespace bloom;

void Window::PollEvents() { glfwPollEvents(); }

void Window::WaitEvents() { glfwWaitEvents(); }

void Window::InitWindowSystem() {
    if (!glfwInit()) {
        Logger::Fatal("Failed to initialize GLFW");
        std::exit(1);
    }
}

std::unique_ptr<Window> Window::Create(WindowDescription const& desc) {
    return std::unique_ptr<Window>(new Window(desc));
}

Window::Window(WindowDescription const& d) {
    desc = WindowDescPrivate{ d };
    desc.fullscreen = false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto* const w = glfwCreateWindow(desc.size.x, desc.size.y,
                                     desc.title.data(), nullptr, nullptr);
    windowPtr = std::unique_ptr<void, Deleter>(w);
    platformInit();
    glfwSetWindowUserPointer(GLFW_WND, this);
    glfwSetWindowPos(GLFW_WND, desc.position.x, desc.position.y);
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
    setSwapchain(device.createSwapchain(swapchainDesc));
}

void Window::setCommandQueue(std::unique_ptr<CommandQueue> queue) {
    _commandQueue = std::move(queue);
}

void Window::endFrame() { userInput.endFrame(); }

void Window::onInput(std::function<void(InputEvent const&)> f) {
    callbacks.onInputFn = std::move(f);
}

void Window::onTextInput(std::function<void(unsigned)> f) {
    callbacks.onCharInputFn = std::move(f);
}

void Window::onFileDrop(
    std::function<void(std::span<std::filesystem::path const>)> f) {
    callbacks.onFileDropFn = std::move(f);
}

void Window::onMove(std::function<void(vml::int2 newPosition)> f) {
    callbacks.onMoveFn = std::move(f);
}

void Window::onResize(std::function<void(vml::int2 newSize)> f) {
    callbacks.onResizeFn = std::move(f);
}

void Window::onFocus(std::function<void()> f) {
    callbacks.onFocusFn = std::move(f);
}

void Window::onFocusLoss(std::function<void()> f) {
    callbacks.onFocusLossFn = std::move(f);
}

void Window::onClose(std::function<void()> f) {
    callbacks.onCloseFn = std::move(f);
}

void Window::onContentScaleChange(
    std::function<void(vml::float2 newContentScale)> f) {
    callbacks.onContentScaleChangeFn = std::move(f);
}

bool Window::shouldClose() const {
    return !desc.shallPreventClose && glfwWindowShouldClose(GLFW_WND);
}

void Window::setTitle(std::string newTitle) {
    desc.title = newTitle;
    glfwSetWindowTitle(GLFW_WND, newTitle.data());
}

void Window::setPosition(vml::int2 newPosition) {
    glfwSetWindowPos(GLFW_WND, newPosition.x, newPosition.y);
    glfwGetWindowPos(GLFW_WND, &desc.position.x, &desc.position.y);
}

void Window::setSize(vml::int2 newSize) {
    newSize.x = std::max(newSize.x, 0);
    newSize.y = std::max(newSize.y, 0);
    glfwSetWindowSize(GLFW_WND, newSize.x, newSize.y);
    glfwGetWindowSize(GLFW_WND, &desc.size.x, &desc.size.y);
}

void Window::setMinSize(vml::int2 size) {
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

void Window::setMaxSize(vml::int2 size) {
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
    auto* monitor = glfwGetPrimaryMonitor();
    auto* vidMode = glfwGetVideoMode(monitor);
    int2 monitorPos = 0;
    int2 monitorSize = { vidMode->width, vidMode->height };
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
    applyStyle();
}

void Window::close() { glfwSetWindowShouldClose(GLFW_WND, true); }

void Window::stopClose() { glfwSetWindowShouldClose(GLFW_WND, false); }

void Window::requestAttention() { glfwRequestWindowAttention(GLFW_WND); }

#if !defined(BLOOM_PLATFORM_APPLE)
void Window::platformInit() {}
#endif

void Window::setCallbacks() {
    /// For Apple we handle mouse events in "CocoaWindow.mm"
#if !defined(BLOOM_PLATFORM_APPLE)
    glfwSetMouseButtonCallback(GLFW_WND, [](GLFWwindow* w, int button,
                                            int action, int mods) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        window.userInput
            ._mouseButtons[(std::size_t)mouseButtonFromGLFW(button)] = action;
        window.userInput._modFlags = modFlagsFromGLFW(mods);
        if (window.callbacks.onInputFn) {
            auto const event =
                makeInputEventFromGLFWMouseButton(window.userInput, button,
                                                  action, mods);
            window.callbacks.onInputFn(event);
        }
    });
#endif
    glfwSetCursorPosCallback(GLFW_WND,
                             [](GLFWwindow* w, double xpos, double ypos) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        window.userInput.setMousePosition({ xpos, ypos });
        if (window.callbacks.onInputFn) {
            window.callbacks.onInputFn(
                makeInputEventFromGLFWCursorPos(window.userInput, xpos, ypos));
        }
    });
    glfwSetCursorEnterCallback(GLFW_WND,
                               [](GLFWwindow* w, [[maybe_unused]] int entered) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
    });
    glfwSetScrollCallback(GLFW_WND,
                          [](GLFWwindow* w, double xoffset, double yoffset) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        window.userInput.setScrollOffset({ xoffset, yoffset });
        if (window.callbacks.onInputFn) {
            window.callbacks.onInputFn(
                makeInputEventFromGLFWScroll(window.userInput, xoffset,
                                             yoffset));
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
        if (window.callbacks.onInputFn) {
            window.callbacks.onInputFn(
                makeInputEventFromGLFWKey(window.userInput, key, scancode,
                                          action, mods));
        }
    });
    glfwSetCharCallback(GLFW_WND, [](GLFWwindow* w, unsigned int codepoint) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        if (window.callbacks.onCharInputFn) {
            window.callbacks.onCharInputFn(codepoint);
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
        if (window.callbacks.onFileDropFn) {
            window.callbacks.onFileDropFn(pathVec);
        }
    });
    glfwSetWindowPosCallback(GLFW_WND, [](GLFWwindow* w, int posx, int posy) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        int2 pos = { posx, posy };
        window.desc.position = pos;
        if (window.callbacks.onMoveFn) {
            window.callbacks.onMoveFn(pos);
        }
    });
    glfwSetWindowSizeCallback(GLFW_WND,
                              [](GLFWwindow* w, int sizex, int sizey) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        int2 newSize = { sizex, sizey };
        window.desc.size = newSize;
        window.resizeSwapchain(newSize);
        if (window.callbacks.onResizePrivateFn) {
            window.callbacks.onResizePrivateFn(newSize);
        }
        if (window.callbacks.onResizeFn) {
            window.callbacks.onResizeFn(newSize);
        }
    });
    glfwSetWindowFocusCallback(GLFW_WND, [](GLFWwindow* w, int focus) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        if (focus == GLFW_TRUE) {
            window.desc.focused = true;
            if (window.callbacks.onFocusFn) {
                window.callbacks.onFocusFn();
            }
        }
        else {
            window.desc.focused = false;
            if (window.callbacks.onFocusLossFn) {
                window.callbacks.onFocusLossFn();
            }
        }
    });
    glfwSetWindowCloseCallback(GLFW_WND, [](GLFWwindow* w) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        window.desc.shallPreventClose = false;
        if (window.callbacks.onCloseFn) {
            window.callbacks.onCloseFn();
        }
    });
    glfwSetWindowContentScaleCallback(GLFW_WND,
                                      [](GLFWwindow* w, float x, float y) {
        [[maybe_unused]] Window& window = *(Window*)glfwGetWindowUserPointer(w);
        float2 contentScale = { x, y };
        window.desc.contentScaleFactor = contentScale;
        if (window.callbacks.onContentScaleChangeFn) {
            window.callbacks.onContentScaleChangeFn(contentScale);
        }
    });
}

void Window::resizeSwapchain(vml::int2 newSize) {
    if (_swapchain && desc.autoResizeSwapchain) {
        swapchain().resize(newSize * contentScaleFactor());
    }
}

void Window::Deleter::operator()(void* ptr) const {
    glfwDestroyWindow((GLFWwindow*)ptr);
}

YAML::Node YAML::convert<WindowDescription>::encode(
    bloom::WindowDescription const& desc) {
    YAML::Node node;
    node["Title"] = desc.title;
    node["Size"] = desc.size;
    node["Position"] = desc.position;
    node["AutoResizeSwapchain"] = desc.autoResizeSwapchain;
    node["Fullscreen"] = desc.fullscreen;
    return node;
}

bool YAML::convert<WindowDescription>::decode(YAML::Node const& node,
                                              WindowDescription& desc) {
    desc.title = node["Title"].as<std::string>();
    desc.size = node["Size"].as<int2>();
    desc.position = node["Position"].as<int2>();
    desc.autoResizeSwapchain = node["AutoResizeSwapchain"].as<bool>();
    desc.fullscreen = node["Fullscreen"].as<bool>();
    return true;
}

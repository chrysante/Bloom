#include "Bloom/Application/Window.h"

#include <optional>

#include <GLFW/glfw3.h>

#include "Bloom/Application/InputInternal.h"
#include "Bloom/Application/WindowInternal.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/Swapchain.h"

#define GLFW_WND (impl->glfwWnd.get())

using namespace vml::short_types;
using namespace bloom;
using Impl = Window::Impl;

void Window::PollEvents() { glfwPollEvents(); }

void Window::WaitEvents() { glfwWaitEvents(); }

void Window::InitWindowSystem() {
    if (!glfwInit()) {
        Logger::Fatal("Failed to initialize GLFW");
        std::exit(1);
    }
}

std::unique_ptr<Window> Window::Create(WindowDescription const& desc) {
    auto window = std::unique_ptr<Window>(new Window(desc));
    window->impl->_self = window->impl;
    return window;
}

void Window::destroy() {
    impl->glfwWnd.reset();
    impl->delegate.reset();
}

Window::Window(WindowDescription const& d): impl(std::make_shared<Impl>()) {
    static_cast<WindowDescription&>(*impl) = d;
    impl->window = this;
    impl->fullscreen = false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FOCUSED, d.focused);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, d.focusOnShow);
    glfwWindowHint(GLFW_CAN_BECOME_KEY, d.canBecomeKey);
    glfwWindowHint(GLFW_CAN_BECOME_MAIN, d.canBecomeMain);
    auto* w = glfwCreateWindow(impl->size.x, impl->size.y, impl->title.data(),
                               nullptr, nullptr);
    impl->glfwWnd.reset(w);
    platformInit();
    glfwSetWindowUserPointer(GLFW_WND, impl.get());
    glfwSetWindowPos(GLFW_WND, impl->position.x, impl->position.y);
    glfwGetWindowSize(GLFW_WND, &impl->size.x, &impl->size.y);
    glfwGetWindowContentScale(GLFW_WND, &impl->contentScaleFactor.x,
                              &impl->contentScaleFactor.y);
    setCallbacks();
}

Window::~Window() { destroy(); }

Impl::~Impl() { invokeDelegates(&WindowDelegate::onDestroy); }

void Window::createDefaultSwapchain(HardwareDevice& device,
                                    std::size_t backbufferCount) {
    SwapchainDescription swapchainDesc;
    swapchainDesc.displaySync = true;
    swapchainDesc.size = size() * contentScaleFactor();
    swapchainDesc.backBufferCount = backbufferCount;
    setSwapchain(device.createSwapchain(swapchainDesc));
}

WindowDelegate* Window::addDelegate(std::unique_ptr<WindowDelegate> delegate) {
    auto* result = delegate.get();
    impl->delegates.push_back(std::move(delegate));
    result->_window = this;
    return result;
}

Reference<Swapchain> Window::swapchain() { return impl->swapchain; }

WindowDescription const& Window::getDescription() const { return *impl; }

std::string const& Window::title() const { return impl->title; }

vml::int2 Window::position() const { return impl->position; }

vml::int2 Window::size() const { return impl->size; }

bool Window::focused() const { return impl->focused; }

vml::float2 Window::contentScaleFactor() const {
    return impl->contentScaleFactor;
}

bool Window::isFullscreen() const { return impl->fullscreen; }

bool Window::isWindowed() const { return !isFullscreen(); }

bool Window::isMoving() const { return impl->isMoving; }

bool Window::requestedResize() const { return impl->requestedResizes > 0; }

bool Window::requestedMove() const { return impl->requestedMoves > 0; }

void Window::setMovable(bool value) { impl->movable = value; }

bool Window::shouldClose() const {
    return !impl->shallPreventClose && glfwWindowShouldClose(GLFW_WND);
}

void Window::setTitle(std::string newTitle) {
    impl->title = newTitle;
    glfwSetWindowTitle(GLFW_WND, newTitle.data());
}

void Window::setPosition(vml::int2 newPosition) {
    if (newPosition == impl->position) {
        return;
    }
    ++impl->requestedMoves;
    glfwSetWindowPos(GLFW_WND, newPosition.x, newPosition.y);
    glfwGetWindowPos(GLFW_WND, &impl->position.x, &impl->position.y);
}

void Window::setSize(vml::int2 newSize) {
    if (newSize == impl->size) {
        return;
    }
    newSize.x = std::max(newSize.x, 0);
    newSize.y = std::max(newSize.y, 0);
    ++impl->requestedResizes;
    glfwSetWindowSize(GLFW_WND, newSize.x, newSize.y);
    glfwGetWindowSize(GLFW_WND, &impl->size.x, &impl->size.y);
}

void Window::setMinSize(vml::int2 size) {
    size.x = std::max(size.x, 0);
    size.y = std::max(size.y, 0);
    impl->minSize = size;
    glfwSetWindowSizeLimits(GLFW_WND,
                            impl->minSize.x > 0 ? impl->minSize.x :
                                                  GLFW_DONT_CARE,
                            impl->minSize.y > 0 ? impl->minSize.y :
                                                  GLFW_DONT_CARE,
                            impl->maxSize.x > 0 ? impl->maxSize.x :
                                                  GLFW_DONT_CARE,
                            impl->maxSize.y > 0 ? impl->maxSize.y :
                                                  GLFW_DONT_CARE);
}

void Window::setMaxSize(vml::int2 size) {
    size.x = std::max(size.x, 0);
    size.y = std::max(size.y, 0);
    impl->maxSize = size;
    glfwSetWindowSizeLimits(GLFW_WND,
                            impl->minSize.x > 0 ? impl->minSize.x :
                                                  GLFW_DONT_CARE,
                            impl->minSize.y > 0 ? impl->minSize.y :
                                                  GLFW_DONT_CARE,
                            impl->maxSize.x > 0 ? impl->maxSize.x :
                                                  GLFW_DONT_CARE,
                            impl->maxSize.y > 0 ? impl->maxSize.y :
                                                  GLFW_DONT_CARE);
}

void Window::setFocused() { glfwFocusWindow(GLFW_WND); }

void Window::setShouldClose(bool value) {
    glfwSetWindowShouldClose(GLFW_WND, value);
}

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
    impl->backupSize = impl->size;
    impl->size = monitorSize;
    impl->backupPosition = impl->position;
    impl->position = monitorPos;
    impl->fullscreen = true;
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
    impl->size = impl->backupSize;
    impl->position = impl->backupPosition;
    impl->fullscreen = false;
    glfwSetWindowMonitor(GLFW_WND, nullptr, impl->position.x, impl->position.y,
                         impl->size.x, impl->size.y, GLFW_DONT_CARE);
    applyStyle();
}

void Window::close() { glfwSetWindowShouldClose(GLFW_WND, true); }

void Window::stopClose() { glfwSetWindowShouldClose(GLFW_WND, false); }

void Window::haltClose() { impl->shallPreventClose = true; }

void Window::requestAttention() { glfwRequestWindowAttention(GLFW_WND); }

#if !defined(BLOOM_PLATFORM_APPLE)
void Window::platformInit() {}
#endif

static Impl& getImpl(GLFWwindow* w) {
    return *static_cast<Impl*>(glfwGetWindowUserPointer(w));
}

void Window::setCallbacks() {
    glfwSetCharCallback(GLFW_WND, [](GLFWwindow* w, unsigned int codepoint) {
        auto& impl = getImpl(w);
        impl.invokeDelegates(&WindowDelegate::onTextInput, codepoint);
    });
    glfwSetDropCallback(GLFW_WND,
                        [](GLFWwindow* w, int pathCount, char const* paths[]) {
        auto& impl = getImpl(w);
        utl::small_vector<std::filesystem::path> pathVec;
        pathVec.reserve(pathCount);
        for (int i = 0; i < pathCount; ++i) {
            pathVec.push_back(std::filesystem::path(paths[i]));
        }
        impl.invokeDelegates(&WindowDelegate::onFileDrop, pathVec);
    });
    static constexpr auto posCallback = [](Impl& impl, int2 pos) {
        impl.position = pos;
        impl.invokeDelegates(&WindowDelegate::onMove, pos);
    };
    glfwSetWindowPosCallback(GLFW_WND, [](GLFWwindow* w, int posx, int posy) {
        auto& impl = getImpl(w);
        posCallback(impl, { posx, posy });
        if (impl.requestedMoves > 0) {
            --impl.requestedMoves;
        }
    });
    glfwSetWindowSizeCallback(GLFW_WND,
                              [](GLFWwindow* w, int sizex, int sizey) {
        auto& impl = getImpl(w);
        int2 newSize = { sizex, sizey };
        impl.size = newSize;
        if (impl.autoResizeSwapchain) {
            impl.resizeSwapchain(newSize);
        }
        int2 position = 0;
        glfwGetWindowPos(w, &position.x, &position.y);
        if (impl.position != position) {
            BL_ASSERT(impl.requestedResizes == 0,
                      "This should not happen for API resize requests");
            posCallback(impl, position);
        }
        impl.invokeDelegates(&WindowDelegate::onResize, newSize);
        if (impl.frameCallback) {
            impl.frameCallback(*impl.window);
        }
        if (impl.requestedResizes > 0) {
            --impl.requestedResizes;
        }
    });
    glfwSetWindowFocusCallback(GLFW_WND, [](GLFWwindow* w, int focus) {
        auto& impl = getImpl(w);
        if (focus == GLFW_TRUE) {
            impl.focused = true;
            impl.invokeDelegates(&WindowDelegate::onFocus);
        }
        else {
            impl.focused = false;
            impl.invokeDelegates(&WindowDelegate::onFocusLoss);
        }
    });
    glfwSetWindowCloseCallback(GLFW_WND, [](GLFWwindow* w) {
        auto& impl = getImpl(w);
        impl.shallPreventClose = false;
        impl.invokeDelegates(&WindowDelegate::onClose);
    });
    glfwSetWindowContentScaleCallback(GLFW_WND,
                                      [](GLFWwindow* w, float x, float y) {
        auto& impl = getImpl(w);
        float2 contentScale = { x, y };
        impl.contentScaleFactor = contentScale;
        impl.invokeDelegates(&WindowDelegate::onContentScaleChange,
                             contentScale);
    });
}

void Window::resizeSwapchain(vml::int2 newSize) {
    impl->resizeSwapchain(newSize);
}

void Impl::resizeSwapchain(vml::int2 newSize) {
    if (swapchain) {
        swapchain->resize(newSize * contentScaleFactor);
    }
}

void Impl::GLFWDeleter::operator()(GLFWwindow* w) const {
    glfwDestroyWindow(w);
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

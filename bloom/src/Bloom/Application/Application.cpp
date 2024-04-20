#include "Bloom/Application/Application.h"

#include <iostream>

#include <range/v3/algorithm.hpp>

#include "Bloom/Application/InputInternal.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Core/Autorelease.h"
#include "Bloom/Core/Debug.h"

using namespace bloom;
using namespace vml::short_types;

class bloom::AppRunner {
public:
    static void run(Application& app) { app.run(); }
};

/// We use `weak` attribute to allow clients to supply their own main function
__attribute__((weak)) int main(int, char*[]) {
    auto app = createApplication();
    AppRunner::run(*app);
}

std::unique_ptr<bloom::Application> bloom::createApplication() {
    std::cerr
        << "Error: bloom::createApplication() must be overridden by client"
        << std::endl;
    std::terminate();
}

struct Application::Impl {
    int inputCallbackID = 0;
};

Application::Application(): mCoreSystems(this), impl(std::make_unique<Impl>()) {
    messageSystem = MessageSystem::Create();
    assignEmitter(makeEmitter());
    assignReceiver(makeReceiver());
    impl->inputCallbackID = registerGlobalInputCallback(
        std::bind_front(&Application::doOnInput, this));
}

Application::~Application() {
    unregisterGlobalInputCallback(impl->inputCallbackID);
}

utl::small_vector<Window*> Application::getWindows() {
    utl::small_vector<Window*> result;
    result.reserve(mWindows.size());
    std::transform(mWindows.begin(), mWindows.end(), std::back_inserter(result),
                   [](auto& wrapper) { return wrapper.window.get(); });
    return result;
}

Emitter Application::makeEmitter() { return messageSystem->makeEmitter(); }

Receiver Application::makeReceiver() { return messageSystem->makeReceiver(); }

Window& Application::createWindow(WindowDescription const& desc,
                                  std::unique_ptr<WindowDelegate> delegate) {
    auto window = Window::Create(desc);
    window->callbacks.onResizePrivateFn = [this](int2) { doFrame(); };
    if (delegate) {
        delegate->wnd = window.get();
        delegate->init();
    }
    mWindows.push_back({ std::move(window), std::move(delegate) });
    return *mWindows.back().window;
}

void Application::destroyWindow(Window* window) {
    auto itr = ranges::remove_if(mWindows, [&](WindowWrapper const& wrapper) {
        return wrapper.window.get() == window;
    });
    mWindows.erase(itr, mWindows.end());
}

static void handleEvents(RunLoopMode mode) {
    using enum RunLoopMode;
    switch (mode) {
    case EventDriven:
        Window::WaitEvents();
        break;
    case Realtime:
        Window::PollEvents();
        break;
    default:
        BL_UNREACHABLE();
    }
}

void Application::run() {
    doInit();
    while (!mWindows.empty()) {
        handleEvents(mRunLoopMode);
        doFrame();
        clearClosingWindows();
    }
    doShutdown();
}

void Application::doInit() {
    registerListeners();
    Window::InitWindowSystem();
    mCoreSystems.init();
    this->init();
    mTimer.reset();
}

void Application::registerListeners() {
    listen([](CustomCommand const& cmd) {
        if (cmd.function) {
            cmd.function();
        }
    });
}

void Application::doShutdown() {
    this->shutdown();
    mCoreSystems.shutdown();
}

void Application::doFrame() {
    messageSystem->flush();
    BLOOM_AUTORELEASE_BEGIN
    this->frame();
    BLOOM_AUTORELEASE_END
    for (auto&& [window, delegate]: mWindows) {
        if (delegate) {
            auto& d = *delegate;
            BLOOM_AUTORELEASE_BEGIN
            d.frame();
            BLOOM_AUTORELEASE_END
        }
        inputEndFrame(mInput);
    }
    mTimer.update();
}

void Application::clearClosingWindows() {
    for (auto itr = mWindows.begin(); itr != mWindows.end();) {
        auto&& [window, delegate] = *itr;
        if (!window->shouldClose()) {
            ++itr;
            continue;
        }
        if (delegate) {
            delegate->shutdown();
        }
        itr = mWindows.erase(itr);
        if (!mWindows.empty()) {
            mWindows.front().window->setFocused();
        }
    }
}

void Application::setRunLoopMode(RunLoopMode mode) { mRunLoopMode = mode; }

#include "Bloom/Application/Application.hpp"

#include <iostream>

#include "Bloom/Application/Window.hpp"
#include "Bloom/Core/Autorelease.hpp"
#include "Bloom/Core/Debug.hpp"

using namespace bloom;
using namespace mtl::short_types;

int main(int argc, char* argv[]) {
    auto theApp = createApplication();
    theApp->run();
}

std::unique_ptr<bloom::Application> bloom::createApplication() {
    std::cerr
        << "Error: bloom::createApplication() must be overridden by client"
        << std::endl;
    std::terminate();
}

Application::Application():
    Receiver(makeReciever()), Emitter(makeEmitter()), mCoreSystems(this) {}

Application::~Application() = default;

/// MARK: Queries

utl::small_vector<Window*> Application::getWindows() {
    utl::small_vector<Window*> result;
    result.reserve(mWindows.size());
    std::transform(mWindows.begin(), mWindows.end(), std::back_inserter(result),
                   [](auto& wrapper) { return wrapper.window.get(); });
    return result;
}

/// MARK: Modifiers

Window& Application::createWindow(WindowDescription const& windowDesc,
                                  std::unique_ptr<WindowDelegate> delegate) {
    auto window = std::make_unique<Window>(windowDesc);
    window->desc.application = this;
    window->onResizePrivateFn = [this](int2 newSize) { doFrame(); };
    window->onMovePrivateFn = [this](int2 newSize) { doFrame(); };
    delegate->theWindow = window.get();
    delegate->init();
    mWindows.push_back({ std::move(window), std::move(delegate) });
    return *mWindows.back().window;
}

/// MARK: Private

void Application::run() {
    doInit();
    while (!mWindows.empty()) {
        doFrame();
        Window::pollEvents();
        clearClosingWindows();
    }
    doShutdown();
}

/// MARK: Lifetime
void Application::doInit() {
    registerListeners();
    Window::initWindowSystem();
    mCoreSystems.init();
    this->init();
    mTimer.reset();
}

void Application::registerListeners() {
    listen([this](CustomCommand const& cmd) {
        if (cmd.function) {
            cmd.function();
        }
    });
}

void Application::doShutdown() {
    this->shutdown();
    mCoreSystems.shutdown();
}

/// MARK: Frame
void Application::doFrame() {
    if (skipFrame) {
        skipFrame = false;
        return;
    }
    MessageSystem::flush();
    BLOOM_AUTORELEASE_BEGIN
    this->frame();
    BLOOM_AUTORELEASE_END
    for (auto&& [window, delegate]: mWindows) {
        window->beginFrame();
        if (delegate) {
            auto& d = *delegate;
            BLOOM_AUTORELEASE_BEGIN
            d.frame();
            BLOOM_AUTORELEASE_END
        }
        window->endFrame();
    }
    mTimer.update();
    std::size_t const frameDuration = mTimer.preciseTimestep().delta.count();
    std::size_t const targetFrameDuration = 16'000'000;
    if (frameDuration < targetFrameDuration) {
        std::this_thread::sleep_for(
            std::chrono::nanoseconds{ targetFrameDuration - frameDuration });
    }
}

void Application::clearClosingWindows() {
    for (auto itr = mWindows.begin(); itr != mWindows.end();) {
        auto&& [window, delegate] = *itr;
        if (!window->shouldClose()) {
            ++itr;
            continue;
        }
        delegate->shutdown();
        itr = mWindows.erase(itr);
        if (!mWindows.empty()) {
            mWindows.front().window->setFocused();
        }
    }
}

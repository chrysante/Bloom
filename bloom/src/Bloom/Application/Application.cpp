#include "Bloom/Application/Application.h"

#include <iostream>

#include <range/v3/algorithm.hpp>

#include "Bloom/Application/InputInternal.h"
#include "Bloom/Application/RenderThread.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Core/Autorelease.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Ranges.h"
#include "Bloom/Core/Utils.h"

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

Application* Application::gInstance = nullptr;

struct Application::Impl {
    int inputCallbackID = 0;
    RenderThread renderThread;
};

Application::Application(): mCoreSystems(this), impl(std::make_unique<Impl>()) {
    gInstance = this;
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
    return mWindows | ranges::views::transform(ToAddress<>) | ToSmallVector<>;
}

Emitter Application::makeEmitter() { return messageSystem->makeEmitter(); }

Receiver Application::makeReceiver() { return messageSystem->makeReceiver(); }

Window& Application::createWindow(WindowDescription desc) {
    if (!desc.frameCallback) {
        desc.frameCallback = [this](Window& window) {
            if (!window.requestedResize()) {
                /// Very important that we don't wait on events when resizing.
                /// Otherwise the application might freeze during resize because
                /// no events are being sent
                doFrame(RunLoopMode::None);
            }
        };
    }
    mWindows.push_back(Window::Create(desc));
    return *mWindows.back();
}

void Application::destroyWindow(Window& window) {
    auto itr =
        ranges::find_if(mWindows, [&](auto& w) { return w.get() == &window; });
    if (itr != mWindows.end()) {
        mWindows.erase(itr);
    }
}

void Application::closeAllWindows() {
    for (auto& window: mWindows) {
        window->setShouldClose();
    }
}

void Application::setRunLoopMode(RunLoopMode mode) { mRunLoopMode = mode; }

void Application::onRenderThread(std::function<void()> callback) {
    impl->renderThread.submit(std::move(callback));
}

static void handleEvents(RunLoopMode mode) {
    using enum RunLoopMode;
    switch (mode) {
    case None:
        break;
    case EventDriven:
        Window::WaitEvents();
        break;
    case Realtime:
        Window::PollEvents();
        break;
    }
}

void Application::run() {
    doInit();
    while (!mWindows.empty()) {
        doFrame(mRunLoopMode);
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

void Application::doFrame(RunLoopMode mode) {
    BLOOM_AUTORELEASE_BEGIN
    handleEvents(mode);
    messageSystem->flush();
    this->frame();
    inputEndFrame(mInput);
    clearClosingWindows();
    mTimer.update();
    BLOOM_AUTORELEASE_END
}

void Application::clearClosingWindows() {
    for (auto itr = mWindows.begin(); itr != mWindows.end();) {
        auto* window = itr->get();
        if (!window->shouldClose()) {
            ++itr;
            continue;
        }
        itr = mWindows.erase(itr);
    }
}

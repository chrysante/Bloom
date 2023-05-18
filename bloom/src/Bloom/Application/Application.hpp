#pragma once

#include <memory>

#include <utl/utility.hpp>

#include "Bloom/Application/Commands.hpp"
#include "Bloom/Application/CoreSystemManager.hpp"
#include "Bloom/Application/MessageSystem.hpp"
#include "Bloom/Application/Window.hpp"
#include "Bloom/Application/WindowDelegate.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"

BLOOM_API int main(int, char const**, char const**);

namespace bloom {

class Application;
class HardwareDevice;

BLOOM_WEAK_API Application* createApplication();

struct CommandLineArgs {
    int argc;
    char const** argv;
    char const** env;
};

struct FrameData {
    std::size_t frameCount;

    std::chrono::nanoseconds time;
    std::chrono::nanoseconds deltaTime;
};

class BLOOM_API Application:
    private MessageSystem,
    protected Reciever,
    protected Emitter {
public:
    Application();
    ~Application();

    /// MARK: Virtual Interface
    virtual void init() {}
    virtual void shutdown() {}
    virtual void frame() {}

    /// MARK: Queries
    CommandLineArgs commandLineArgs() const { return cmdLineArgs; }

    /// \Returns a reference to the current GPU
    HardwareDevice& device() { return coreSystems().device(); }

    CoreSystemManager& coreSystems() { return mCoreSystems; }

    Timestep time() const { return mTimer.timestep(); }

    utl::small_vector<Window*> getWindows();

    using MessageSystem::makeEmitter;

    using MessageSystem::makeReciever;

    /// MARK: Modifiers
    Window& createWindow(WindowDescription const&,
                         std::unique_ptr<WindowDelegate> = nullptr);
    Window& createWindow(WindowDescription const&, WindowDelegate*);

private:
    friend int ::main(int, char const**, char const**);

    void run();
    void doInit();
    void registerListeners();
    void doShutdown();
    void doFrame();
    void clearClosingWindows();

    CommandLineArgs cmdLineArgs{};
    utl::vector<std::pair<std::unique_ptr<bloom::Window>,
                          std::unique_ptr<bloom::WindowDelegate>>>
        mWindows;
    CoreSystemManager mCoreSystems;
    Timer mTimer;
    std::size_t mFrameCounter = 0;
    bool skipFrame = false; // used to skip frames on resize and move events to
                            // avoid visual glitches
};

} // namespace bloom

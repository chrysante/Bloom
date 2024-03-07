#ifndef BLOOM_APPLICATION_APPLICATION_H
#define BLOOM_APPLICATION_APPLICATION_H

#include <memory>

#include <utl/utility.hpp>

#include "Bloom/Application/CoreSystemManager.hpp"
#include "Bloom/Application/MessageSystem.hpp"
#include "Bloom/Application/Window.hpp"
#include "Bloom/Application/WindowDelegate.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"

/// Bloom provides the main function
BLOOM_API int main(int argc, char* argv[]);

namespace bloom {

class Application;
class HardwareDevice;

/// Users are expected to provide an implementation of this function in their
/// code, that returns a derived application
BLOOM_WEAK_API std::unique_ptr<Application> createApplication();

/// Base class of user application. Users are expected to derive from this class
/// to implement their application
class BLOOM_API Application:
    private MessageSystem,
    protected Receiver,
    protected Emitter {
public:
    /// MARK: Virtual Interface
    ///
    virtual ~Application();

    /// Called by the framework after core systems have been initialized.
    virtual void init() {}

    /// Called before the destructor is run
    /// TODO: Remove this, destructor should be enough
    virtual void shutdown() {}

    /// Called every frame after the message system has been flushed and before
    /// the frame delegates of the open windows are called
    virtual void frame() {}

    /// \Returns a reference to the current GPU
    HardwareDevice& device() { return coreSystems().device(); }

    /// \Returns the core system manager
    CoreSystemManager& coreSystems() { return mCoreSystems; }

    /// \Returns the current timestep
    Timestep time() const { return mTimer.timestep(); }

    ///
    utl::small_vector<Window*> getWindows();

    using MessageSystem::makeEmitter;

    using MessageSystem::makeReciever;

    /// Creates a new window with from window description \p desc and window
    /// delegate \p delegate
    Window& createWindow(WindowDescription const& desc,
                         std::unique_ptr<WindowDelegate> delegate = nullptr);

protected:
    ///
    Application();

private:
    friend int ::main(int, char*[]);

    /// Main function of the application. Runs initialization, main loop and
    /// shutdown
    void run();

    ///
    void doInit();

    ///
    void registerListeners();

    ///
    void doShutdown();

    ///
    void doFrame();

    ///
    void clearClosingWindows();

    /// Owns a window and a corresponding window delegate
    struct WindowWrapper {
        std::unique_ptr<bloom::Window> window;
        std::unique_ptr<bloom::WindowDelegate> delegate;
    };

    utl::vector<WindowWrapper> mWindows;
    CoreSystemManager mCoreSystems;
    Timer mTimer;
    std::size_t mFrameCounter = 0;
    bool skipFrame = false; // used to skip frames on resize and move events to
                            // avoid visual glitches
};

} // namespace bloom

#endif // BLOOM_APPLICATION_APPLICATION_H

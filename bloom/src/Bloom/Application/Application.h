#ifndef BLOOM_APPLICATION_APPLICATION_H
#define BLOOM_APPLICATION_APPLICATION_H

#include <memory>
#include <vector>

#include <utl/utility.hpp>

#include "Bloom/Application/CoreSystemManager.h"
#include "Bloom/Application/Input.h"
#include "Bloom/Application/MessageSystem.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Time.h"

namespace bloom {

class Application;
class HardwareDevice;
class AppRunner;
class MessageSystem;
class Emitter;
class Receiver;

/// Users are expected to provide an implementation of this function in their
/// code, that returns a derived application
BLOOM_WEAK_API std::unique_ptr<Application> createApplication();

/// Run loop modes for the application
enum class RunLoopMode {
    /// Runs the main loop body after recieving input or other events
    EventDriven,

    /// Continuously runs the main looo body regardless of input or other events
    Realtime
};

/// Base class of user application. Users are expected to derive from this class
/// to implement their application
class BLOOM_API Application: protected Receiver, public Emitter {
public:
    Application(Application const&) = delete;
    Application& operator=(Application const&) = delete;

    virtual ~Application();

    /// \Returns a reference to the current GPU
    HardwareDevice& device() { return coreSystems().device(); }

    /// \Returns a reference to the applicatins command queue
    CommandQueue& commandQueue() { return coreSystems().commandQueue(); }

    /// \Returns the core system manager
    CoreSystemManager& coreSystems() { return mCoreSystems; }

    /// \overload
    CoreSystemManager const& coreSystems() const { return mCoreSystems; }

    /// \Returns the current timestep
    Timestep time() const { return mTimer.timestep(); }

    ///
    utl::small_vector<Window*> getWindows();

    /// Creates an emitter that emits to the message system of this application
    Emitter makeEmitter();

    /// Creates an receiver that listens to the message system of this
    /// application
    Receiver makeReceiver();

    /// Creates a new window with from window description \p desc and window
    /// delegate \p delegate
    Window& createWindow(WindowDescription const& desc);

    /// Immediately destroy \p window
    void destroyWindow(Window& window);

    /// Sets the close flag on all windows
    void closeAllWindows();

    /// \Returns the application global input reference
    Input const& input() const { return mInput; }

    /// Executes \p callback on the render thread
    void onRenderThread(std::function<void()> callback);

protected:
    ///
    Application();

    /// Sets the run loop mode of this application. See `RunLoopMode` for more
    /// details Defaults to `RunLoopMode::Realtime`
    void setRunLoopMode(RunLoopMode mode);

private:
    struct Impl;

    /// Called by the framework after core systems have been initialized.
    virtual void init() {}

    /// Called before the destructor is run
    /// TODO: Remove this, destructor should be enough
    virtual void shutdown() {}

    /// Called every frame after the message system has been flushed and before
    /// the frame delegates of the open windows are called
    virtual void frame() {}

    /// Called for every input event that occurs in any window
    /// \Returns `true` if the event has been handled
    virtual bool onInput(InputEvent) { return false; }

    friend class AppRunner;

    /// Main function of the application. Runs initialization, main loop and
    /// shutdown
    void run();
    void doInit();
    void registerListeners();
    void doShutdown();
    void doFrame(bool handleEvents = true);
    void clearClosingWindows();
    void doOnInput(InputEvent const& event, void* native);

    std::shared_ptr<MessageSystem> messageSystem;
    std::vector<std::unique_ptr<Window>> mWindows;
    CoreSystemManager mCoreSystems;
    Timer mTimer;
    RunLoopMode mRunLoopMode = RunLoopMode::Realtime;
    Input mInput;
    std::unique_ptr<Impl> impl;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_APPLICATION_H

#ifndef BLOOM_APPLICATION_WINDOW_H
#define BLOOM_APPLICATION_WINDOW_H

#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>

#include <mtl/mtl.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application/Input.h"
#include "Bloom/Application/InputEvent.h"
#include "Bloom/Core/Base.h"
#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/Swapchain.h"

namespace bloom {

class HardwareDevice;
class Application;

///
BLOOM_API void showSaveView(std::function<void(std::string)> completion);

///
BLOOM_API void showOpenView(std::function<void(std::string)> completion);

/// Constructor arguments for a window
struct WindowDescription {
    /// The window title
    std::string title;

    /// The initial size of the window
    mtl::int2 size = 0;

    /// Set to true to make the window automatically resize the swapchain when
    /// window size changes
    bool autoResizeSwapchain = true;

    /// This is ignored for now
    bool fullscreen = false;
};

/// Wraps a native window handle
class BLOOM_API Window {
    friend class Application;

public:
    /// Sets up the application as a windowed application. Must be called before
    /// creating any windows.
    static void InitWindowSystem();

    /// Polls OS event like input and window movement and returns after calling
    /// event callbacks
    static void PollEvents();

    /// Waits for OS events like input and window movement and returns after
    /// calling event callbacks
    static void WaitEvents();

    /// Creates and shows a window with description \p desc
    static std::unique_ptr<Window> Create(WindowDescription const& desc);

    /// Windows are not copyable
    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    ///
    ~Window();

    /// Creates a swapchain with default settings and sets it as this windows
    /// swapchain
    void createDefaultSwapchain(HardwareDevice& device,
                                std::size_t backbufferCount = 3);

    /// Sets \p swapchain as this windows swapchain
    void setSwapchain(std::unique_ptr<Swapchain> swapchain);

    /// Sets \p commandQueue as this windows command queue
    void setCommandQueue(std::unique_ptr<CommandQueue> commandQueue);

    /// Resets the scroll and mouse offset in the input class so this must be
    /// called after each frame. This is not pretty but we go with it for now.
    void endFrame();

    /// Sets the input callback to \p callback
    void onInput(std::function<void(InputEvent const& event)> callback);

    /// Sets the text input callback to \p callback
    void onTextInput(std::function<void(unsigned int)>);

    /// Sets the file drop callback to \p callback
    void onFileDrop(
        std::function<void(std::span<std::filesystem::path const>)>);

    /// Sets the move callback to \p callback
    void onMove(std::function<void(mtl::int2 newPosition)>);

    /// Sets the resize callback to \p callback
    void onResize(std::function<void(mtl::int2 newSize)>);

    /// Sets the focus gain callback to \p callback
    void onFocus(std::function<void()>);

    /// Sets the focus loss callback to \p callback
    void onFocusLoss(std::function<void()>);

    /// Sets the close loss callback to \p callback
    void onClose(std::function<void()>);

    /// Sets the content scale change loss callback to \p callback
    void onContentScaleChange(std::function<void(mtl::float2 newContentScale)>);

    /// \Returns the window's swapchain
    Swapchain& swapchain() { return *_swapchain; }

    /// \Returns the window's command queue
    CommandQueue& commandQueue() { return *_commandQueue; }

    /// \Returns the window's input
    Input const& input() const { return userInput; }

    /// \Returns the window title
    std::string const& title() const { return desc.title; }

    /// \Returns the window's position on the monitor
    mtl::int2 position() const { return desc.position; }

    /// \Returns the window's size
    mtl::int2 size() const { return desc.size; }

    /// \Returns `true` if this window is currently focused
    bool focused() const { return desc.focused; }

    /// \Returns the window's contant scale factor
    mtl::float2 contentScaleFactor() const { return desc.contentScaleFactor; }

    /// \Returns `true` if this window is in fullscreen mode
    bool isFullscreen() const { return desc.fullscreen; }

    /// \Returns `true` if this window is not in fullscreen mode
    bool isWindowed() const { return !isFullscreen(); }

    /// \Returns true if the window's close button has been pressed
    bool shouldClose() const;

    /// \Returns the underlying OS window handle
    void* nativeHandle();

    /// # Experimental and hacky "pretty window" API for MacOS
    /// @{

    /// \Returns the height of the window tool bar in MacOS
    float toolbarHeight() const;

    /// \Returns the bounding box of the close, minimize and maximize buttons
    mtl::AABB<float, 2> titleButtonsArea() const;

    /// Flag to indicate if the mouse is over an area from which the window may
    /// be dragged. We need this because ImGui is not event based but Cocoa
    /// wants to dispatch the mouse event to the window server directly
    void setMovable(bool value = true) { desc.movable = value; }

    /// Activate double click on titlebar behaviour
    void zoom();

    /// @}

    /// A bunch of setters and commands
    /// @{
    void setTitle(std::string title);
    void setPosition(mtl::int2 position);
    void setSize(mtl::int2 size);
    void setMinSize(mtl::int2 minSize);
    void setMaxSize(mtl::int2 maxSize);
    void setFocused();
    void show();
    void hide();
    void maximize();
    void minimize();
    void restore();
    void makeFullscreen();
    void toggleFullscreen();
    void makeWindowed();
    void close();
    void stopClose();
    void haltClose() { desc.shallPreventClose = true; }
    void requestAttention();
    /// @}

private:
    struct Deleter {
        void operator()(void*) const;
    };

    struct WindowDescPrivate: WindowDescription {
        mtl::int2 position = 0;
        mtl::int2 backupPosition = 0;
        mtl::int2 backupSize = 0;
        mtl::float2 contentScaleFactor = 1;
        mtl::int2 minSize = 0;
        mtl::int2 maxSize = 0;
        bool focused = false;
        bool shallPreventClose = false;
        /// See `setMovable()`
        bool movable = true;
    };

    struct Callbacks {
        std::function<void(InputEvent)> onInputFn;
        std::function<void(unsigned)> onCharInputFn;
        std::function<void(std::span<std::filesystem::path const>)>
            onFileDropFn;
        std::function<void(mtl::int2 newPosition)> onMoveFn;
        std::function<void(mtl::int2 newSize)> onResizeFn;
        /// Used by the application to run the main loop during resizing
        std::function<void(mtl::int2 newPosition)> onResizePrivateFn;
        std::function<void()> onFocusFn;
        std::function<void()> onFocusLossFn;
        std::function<void()> onCloseFn;
        std::function<void(mtl::float2 newContentScale)> onContentScaleChangeFn;
    };

    Window(WindowDescription const& desc);
    void platformInit();
    void setCallbacks();
    void resizeSwapchain(mtl::int2 size);

    WindowDescPrivate desc;
    std::unique_ptr<void, Deleter> windowPtr;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<CommandQueue> _commandQueue;
    Input userInput;
    Callbacks callbacks;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_WINDOW_H

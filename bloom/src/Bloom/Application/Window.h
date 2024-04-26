#ifndef BLOOM_APPLICATION_WINDOW_H
#define BLOOM_APPLICATION_WINDOW_H

#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>

#include <utl/utility.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

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
    vml::int2 size = 0;

    /// The initial position of the window
    vml::int2 position = 0;

    /// If set to `true` the window will be created without a titlebar and
    /// without window buttons
    bool noDecoration = false;

    /// If set to `true` the title bar will be larger
    bool hasToolbarArea = false;

    /// Set to true to make the window automatically resize the swapchain when
    /// window size changes
    bool autoResizeSwapchain = true;

    /// This is ignored for now
    bool fullscreen = false;

    /// If set to `true` the window will be created focused
    bool focused = true;

    /// If set to `true` the window will be focused when `.show()` is called
    bool focusOnShow = true;

    /// If set to `true` the window can receive key events when focused
    bool canBecomeKey = true;

    /// If set to `true` the window can become the main window by clicking on it
    bool canBecomeMain = true;
};

/// Provides user callbacks for window events
class BLOOM_API WindowDelegate {
public:
    virtual ~WindowDelegate() = default;

    /// Called for every input event that occurs in this window
    virtual void onInput([[maybe_unused]] InputEvent event) {}

    /// Called on text input
    virtual void onTextInput([[maybe_unused]] unsigned code) {}

    /// Called when the user drops files into the window
    virtual void onFileDrop(
        [[maybe_unused]] std::span<std::filesystem::path const> paths) {}

    /// Called when the window position changes
    virtual void onMove([[maybe_unused]] vml::int2 newPosition) {}

    /// Called when the user starts moving the window
    virtual void onMoveStart() {}

    /// Called when the window size changes
    virtual void onResize([[maybe_unused]] vml::int2 newSize) {}

    /// Called when the window is focused
    virtual void onFocus() {}

    /// Called when the window loses focus
    virtual void onFocusLoss() {}

    /// Called when the user closes the window
    virtual void onClose() {}

    /// Called when the content scale factor changes
    virtual void onContentScaleChange(
        [[maybe_unused]] vml::float2 newContentScale) {}

protected:
    /// \Returns the window that this delegate is attached to
    Window* window() const { return _window; };

private:
    friend class Window;

    Window* _window;
};

/// Wraps a native window handle
class BLOOM_API Window {
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

    /// Adds the window delegate \p delegate to this window.
    /// \Returns a pointer to \p delegate
    WindowDelegate* addDelegate(std::unique_ptr<WindowDelegate> delegate);

    /// \Returns the window's swapchain
    Swapchain* swapchain() { return _swapchain.get(); }

    /// \Returns the window's command queue
    CommandQueue* commandQueue() { return _commandQueue.get(); }

    /// \Returns the window's descriptor of the current state
    WindowDescription const& getDescription() const { return desc; }

    /// \Returns the window title
    std::string const& title() const { return desc.title; }

    /// \Returns the window's position on the monitor
    vml::int2 position() const { return desc.position; }

    /// \Returns the window's size
    vml::int2 size() const { return desc.size; }

    /// \Returns `true` if this window is currently focused
    bool focused() const { return desc.focused; }

    /// \Returns the window's contant scale factor
    vml::float2 contentScaleFactor() const { return desc.contentScaleFactor; }

    /// \Returns `true` if this window is in fullscreen mode
    bool isFullscreen() const { return desc.fullscreen; }

    /// \Returns `true` if this window is not in fullscreen mode
    bool isWindowed() const { return !isFullscreen(); }

    /// \Returns `true` if the window is miniaturized
    bool isMiniaturized() const;

    /// \Returns true if the window's close button has been pressed
    bool shouldClose() const;

    /// \Returns `true` if the window is currently being moved by the user
    bool isMoving() const { return desc.isMoving; }

    /// \Returns the underlying OS window handle
    void* nativeHandle();

    /// # Experimental and hacky "pretty window" API for MacOS
    /// @{

    /// \Returns the height of the window's title bar
    float titlebarHeight() const;

    /// \Returns the bounding box of the close, minimize and maximize buttons in
    /// window coordinates
    vml::AABB<float, 2> titleButtonsArea() const;

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
    void setPosition(vml::int2 position);
    void setSize(vml::int2 size);
    void setMinSize(vml::int2 minSize);
    void setMaxSize(vml::int2 maxSize);
    void addToolbarArea();
    void removeToolbarArea();
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
    friend class Application;

    struct Impl;

    struct Deleter {
        void operator()(void*) const;
    };

    struct WindowDescPrivate: WindowDescription {
        /// Backup positition and size to restore from fullscreen
        vml::int2 backupPosition = 0;
        vml::int2 backupSize = 0;
        vml::float2 contentScaleFactor = 1;
        vml::int2 minSize = 0;
        vml::int2 maxSize = 0;
        bool focused = false;
        bool shallPreventClose = false;
        /// See `setMovable()`
        bool movable = true;
        /// We set this flag when we call `performWindowDragWithEvent` and in
        /// the window position callback and clear it whenever the left mouse
        /// button is released
        bool isMoving = false;
    };

    Window(WindowDescription const& desc);
    void platformInit();
    void applyStyle();
    void setCallbacks();
    void resizeSwapchain(vml::int2 size);
    void onInputImpl(InputEvent const& event, void* native);
    template <typename... Args, typename... Args2>
    void invokeDelegates(void (WindowDelegate::*callback)(Args...),
                         Args2&&... args) const {
        for (auto& del: delegates) {
            (del.get()->*callback)(std::forward<Args2>(args)...);
        }
    }

    WindowDescPrivate desc;
    std::unique_ptr<void, Deleter> windowPtr;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<CommandQueue> _commandQueue;
    std::vector<std::unique_ptr<WindowDelegate>> delegates;
};

} // namespace bloom

template <>
struct YAML::convert<bloom::WindowDescription> {
    static Node encode(bloom::WindowDescription const& desc);
    static bool decode(Node const& node, bloom::WindowDescription& desc);
};

#endif // BLOOM_APPLICATION_WINDOW_H

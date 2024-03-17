#ifndef BLOOM_APPLICATION_WINDOW_H
#define BLOOM_APPLICATION_WINDOW_H

#include <filesystem>
#include <memory>

#include <mtl/mtl.hpp>
#include <string>
#include <utl/functional.hpp>
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
BLOOM_API void showSaveView(utl::function<void(std::string)> completion);

///
BLOOM_API void showOpenView(utl::function<void(std::string)> completion);

/// Constructor arguments for a window
struct WindowDescription {
    /// The window title
    std::string title;

    /// The initial size of the window
    mtl::int2 size = 0;

    ///
    bool autoResizeSwapchain = true;

    /// This is ignored for now
    bool fullscreen = false;
};

///
class BLOOM_API Window {
    friend class Application;

public:
    /// Must be called before creating any windows
    static void initWindowSystem();

    /// Must be called in regular intervals to handle window interaction
    static void pollEvents();

public:
    ///
    Window(WindowDescription const& desc);

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;
    ~Window();

    void createDefaultSwapchain(HardwareDevice&,
                                std::size_t backbufferCount = 2);
    void setSwapchain(std::unique_ptr<Swapchain>);
    void setCommandQueue(std::unique_ptr<CommandQueue>);

    void beginFrame();
    void endFrame();

    void onInput(utl::function<void(InputEvent const&)>);
    void onCharInput(utl::function<void(unsigned int)>);
    void onFileDrop(
        utl::function<void(utl::vector<std::filesystem::path> const&)>);

    void onMove(utl::function<void(mtl::int2 newPosition)>);
    void onResize(utl::function<void(mtl::int2 newSize)>);
    void onFocus(utl::function<void()>);
    void onFocusLoss(utl::function<void()>);

    void onClose(utl::function<void()>);

    void onContentScaleChange(utl::function<void(mtl::float2 newContentScale)>);

    Swapchain& swapchain() { return *_swapchain; }
    CommandQueue& commandQueue() { return *_commandQueue; }
    Input const& input() const { return userInput; }

    std::string_view title() const { return desc.title; }
    mtl::int2 position() const { return desc.position; }
    mtl::int2 size() const { return desc.size; }
    bool focused() const { return desc.focused; }
    mtl::float2 contentScaleFactor() const { return desc.contentScaleFactor; }

    bool isFullscreen() const { return desc.fullscreen; }
    bool isWindowed() const { return !isFullscreen(); }

    bool shouldClose() const;
    void* nativeHandle();

    /// MARK: Modifiers
    void setTitle(std::string);
    void setPosition(mtl::int2);
    void setSize(mtl::int2);
    void setMinSize(mtl::int2);
    void setMaxSize(mtl::int2);
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

private:
    void platformInit();
    void setCallbacks();
    void resizeSwapchain(mtl::int2 size);

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
    };

    WindowDescPrivate desc;
    std::unique_ptr<void, Deleter> glfwWindowPtr;
    std::unique_ptr<Swapchain> _swapchain;
    std::unique_ptr<CommandQueue> _commandQueue;
    Input userInput;
    utl::function<void(InputEvent)> onInputFn;
    utl::function<void(unsigned)> onCharInputFn;
    utl::function<void(utl::vector<std::filesystem::path> const&)> onFileDropFn;
    utl::function<void(mtl::int2 newPosition)> onMoveFn;
    utl::function<void(mtl::int2 newSize)> onResizeFn;
    /// Used by the application to run the main loop during resizing
    utl::function<void(mtl::int2 newPosition)> onResizePrivateFn;
    utl::function<void()> onFocusFn;
    utl::function<void()> onFocusLossFn;
    utl::function<void()> onCloseFn;
    utl::function<void(mtl::float2 newContentScale)> onContentScaleChangeFn;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_WINDOW_H

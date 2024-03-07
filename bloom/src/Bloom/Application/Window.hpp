#pragma once

#include <filesystem>
#include <memory>

#include <mtl/mtl.hpp>
#include <string>
#include <utl/functional.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application/Input.hpp"
#include "Bloom/Application/InputEvent.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/GPU/CommandQueue.hpp"
#include "Bloom/GPU/Swapchain.hpp"

namespace bloom {

class HardwareDevice;
class Application;

BLOOM_API void showSaveView(utl::function<void(std::string)> completion);
BLOOM_API void showOpenView(utl::function<void(std::string)> completion);
//	BLOOM_API void showSelectDirectoryView(utl::function<void(std::string)>
// completion);

/// MARK: WindowDescription
struct WindowDescription {
    std::string title;
    mtl::int2 size = 0;

    mtl::int2 swapchainResizePadding = 100;
    bool autoResizeSwapchain = true;

    bool fullscreen = false; // ignored for now
};

/// MARK: Window Class
class BLOOM_API Window {
    friend class Application;

public:
    /// MARK: Statics
    static void initWindowSystem();
    static void pollEvents();

public:
    /// MARK: Initialization
    Window(WindowDescription const&);
    Window(Window&&) = delete; // because we rely on 'this' being constant for
                               // the lifetime of the window
    ~Window();

    void createDefaultSwapchain(HardwareDevice&,
                                std::size_t backbufferCount = 2);
    void setSwapchain(std::unique_ptr<Swapchain>);
    void setCommandQueue(std::unique_ptr<CommandQueue>);

    /// MARK: Update
    void beginFrame();
    void endFrame();

    /// MARK: Callbacks
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

    /// MARK: Queries
    Application& application() { return *desc.application; }
    Swapchain& swapchain() { return *theSwapchain; }
    CommandQueue& commandQueue() { return *theCommandQueue; }
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
        Application* application = nullptr;
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
    std::unique_ptr<Swapchain> theSwapchain;
    std::unique_ptr<CommandQueue> theCommandQueue;

    Input userInput;

    utl::function<void(InputEvent)> onInputFn;
    utl::function<void(unsigned)> onCharInputFn;
    utl::function<void(utl::vector<std::filesystem::path> const&)> onFileDropFn;

    utl::function<void(mtl::int2 newPosition)> onMoveFn;
    utl::function<void(mtl::int2 newPosition)> onMovePrivateFn;
    utl::function<void(mtl::int2 newSize)> onResizeFn;
    utl::function<void(mtl::int2 newPosition)> onResizePrivateFn;

    utl::function<void()> onFocusFn;
    utl::function<void()> onFocusLossFn;

    utl::function<void()> onCloseFn;

    utl::function<void(mtl::float2 newContentScale)> onContentScaleChangeFn;
};

} // namespace bloom

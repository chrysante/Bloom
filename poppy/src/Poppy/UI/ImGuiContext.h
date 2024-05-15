#ifndef POPPY_UI_IMGUICONTEXT_H
#define POPPY_UI_IMGUICONTEXT_H

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>

#include <imgui.h>
#include <utl/hashmap.hpp>
#include <utl/vector.hpp>
#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui_te_engine.h>
#endif

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Application/MessageSystem.h"
#include "Poppy/Core/Common.h"
#include "Poppy/UI/Font.h"

namespace bloom {

class HardwareDevice;
class Window;

} // namespace bloom

namespace poppy {

class Editor;

struct ImGuiContextDescription {
    std::filesystem::path iniFilePath;
};

class ImGuiContext: bloom::Receiver {
public:
    ~ImGuiContext();
    ///
    void init(Editor&, ImGuiContextDescription const&);
    ///
    void shutdown();
    /// Must be called at the start of each new frame
    void newFrame();
    /// Must be called at the end of each new frame
    void endFrame();
    /// Renders the last drawn frame. Calling this is optional
    void drawFrame();

    void onInput(bloom::InputEvent);
    void onTextInput(unsigned int);

private:
    struct BackendImpl;
    friend class Editor;

    void initPlatform();
    void shutdownPlatform();
    void newFramePlatform();
    void drawFramePlatform();
    void uploadCurrentFontAtlas(bloom::HardwareDevice& device);

    void initTestEngine();
    void stopTestEngine();
    void shutdownTestEngine();

    Editor* mEditor = nullptr;
    ::ImGuiContext* context = nullptr;
    ImGuiContextDescription desc;
    std::mutex drawMutex;
    bool drawDataValid = false;
    std::atomic_int numRenderTasks;
    ImFontAtlas* newFontAtlas = nullptr;
    std::vector<std::any> renderHazardObjects;
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiTestEngine* testEngine;
    bool showTestUI = false;
    bloom::Emitter testEmitter;
#endif
};

} // namespace poppy

#endif // POPPY_UI_IMGUICONTEXT_H

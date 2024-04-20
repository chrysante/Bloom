#ifndef POPPY_UI_IMGUICONTEXT_H
#define POPPY_UI_IMGUICONTEXT_H

#include <filesystem>
#include <memory>

#include <imgui.h>
#include <utl/hashmap.hpp>
#include <utl/vector.hpp>

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
    void init(Editor&, ImGuiContextDescription const&);
    void shutdown();
    void newFrame();
    void drawFrame();

    void onInput(bloom::InputEvent);
    void onTextInput(unsigned int);

private:
    void doInitPlatform();
    void doShutdownPlatform();
    void doNewFramePlatform();
    void doDrawFramePlatform();
    void uploadCurrentFontAtlas(bloom::HardwareDevice& device);

    Editor* mEditor = nullptr;
    ::ImGuiContext* context = nullptr;
    ImGuiContextDescription desc;
};

} // namespace poppy

#endif // POPPY_UI_IMGUICONTEXT_H

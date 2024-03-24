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

struct ImGuiContextDescription {
    std::filesystem::path iniFilePath;
};

class ImGuiContext: bloom::Receiver {
public:
    ~ImGuiContext();
    void init(bloom::Application&, ImGuiContextDescription const&);
    void shutdown();
    void newFrame(bloom::Window&);
    void drawFrame(bloom::HardwareDevice&, bloom::Window&);

    void onInput(bloom::InputEvent);
    void onTextInput(unsigned int);

private:
    void doInitPlatform(bloom::HardwareDevice& device, bloom::Window& window);
    void doShutdownPlatform();
    void doNewFramePlatform(bloom::Window& window);
    void doDrawFramePlatform(bloom::HardwareDevice& device,
                             bloom::Window& window);
    void loadFonts(bloom::HardwareDevice& device, float scaleFactor);
    void createFontAtlasPlatform(ImFontAtlas* fontAtlas,
                                 bloom::HardwareDevice& device);

    bloom::Application* mApplication = nullptr;
    static ImFontAtlas* sFontAtlas;
    ::ImGuiContext* context = nullptr;
    ImGuiContextDescription desc;
};

struct ReloadFontAtlasCommand {};

} // namespace poppy

#endif // POPPY_UI_IMGUICONTEXT_H

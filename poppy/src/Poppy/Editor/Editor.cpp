#include "Poppy/Editor/Editor.h"

#include <fstream>

#include <imgui.h>
#include <range/v3/view.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Autorelease.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Renderer/EditorRenderer.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;
using namespace ranges::views;

/// \Returns the duration in seconds after which the current editor config is
/// automatically saved
static float saveConfigInterval() { return 5; }

std::unique_ptr<Application> bloom::createApplication() {
    return std::make_unique<poppy::Editor>();
}

namespace {

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x)      STRINGIFY_IMPL(x)

struct ToolbarBuildTimeStamp: ToolbarItem {
    static constexpr auto BuildConfig = STRINGIFY(BUILD_CONFIG);
    static constexpr auto Timestamp = __DATE__ " " __TIME__;

    virtual void display(ToolbarStyle const& style) override {
        float2 pos = ImGui::GetCursorPos();
        if (height >= 0) {
            pos.y += (style.actualHeight - height) / 2;
        }
        width = 0;
        ImGui::PushStyleColor(ImGuiCol_Text,
                              ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        auto defFontDesc = FontDesc::UIDefault();
        withFont(defFontDesc.setWeight(FontWeight::Semibold), [&] {
            width = std::max(width, ImGui::CalcTextSize(BuildConfig).x);
            ImGui::SetCursorPos(pos + float2(getWidth() - width, 0));
            ImGui::Text("%s", BuildConfig);
        });
        withFont(defFontDesc.setSize(defFontDesc.size - 4), [&] {
            width = std::max(width, ImGui::CalcTextSize(Timestamp).x);
            ImGui::SetCursorPos(pos + float2(0, (float)defFontDesc.size + 2));
            ImGui::Text("%s", Timestamp);
        });
        height = ImGui::GetCursorPosY() - pos.y;
        ImGui::PopStyleColor();
    }

    virtual float calcWidth(float /* height */) const override { return width; }

    float width = 0;
    float height = -1;
};

} // namespace

Editor::Editor(): viewManager(this), mSelection(makeReceiver()) {
    setRunLoopMode(RunLoopMode::EventDriven);
    cmd.add("Save", { Key::S, ModFlags::Control }, [this] { saveAll(); });
    cmd.add("Simulate", { Key::R, ModFlags::Control },
            [this] { startSimulation(); }, [this] {
        return !coreSystems().sceneSystem().scenes().empty() && !isSimulating();
    });
    cmd.add("Stop Simulation", { Key::Period, ModFlags::Control },
            [this] { stopSimulation(); }, [this] { return isSimulating(); });
    // clang-format off
    dockspace.setLeftToolbar({
        /// Very annoying hack to prevent the toolbar from rendering under the
        /// window buttons on MacOS
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([this] {
                menuBar();
            }),
        ToolbarSpacer(),
        ToolbarIconButton(cmd.find("Stop Simulation"), "stop"),
        ToolbarIconButton(cmd.find("Simulate"), "play")
    });
    dockspace.setCenterToolbar({
        ToolbarIconButton("bank")
            .onClick([] {
                Logger::Trace("Some Button Pressed");
            }),
        ToolbarSpacer(),
        ToolbarIconButton("cw")
            .onClick([&] {
                dispatch(DispatchToken::NextFrame, ReloadShadersCommand{});
            })
            .tooltip("Reload Shaders")
    });
    dockspace.setRightToolbar({
        ToolbarIconButton("attention-circled")
            .onClick([]{ Logger::Trace("Some Button Clicked"); }),
        ToolbarSpacer{},
        ToolbarBuildTimeStamp(),
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([] {
                ImGui::Text("Placeholder Menu");
            }),
    });
    // clang-format on
}

void Editor::openView(std::string name, std::function<void(View&)> completion) {
    dispatch(DispatchToken::NextFrame, [=, this] {
        auto* view = viewManager.openView(name);
        if (view && completion) {
            completion(*view);
        }
    });
}

void Editor::startSimulation() { coreSystems().runtime().run(); }

void Editor::stopSimulation() { coreSystems().runtime().stop(); }

bool Editor::isSimulating() const {
    return coreSystems().runtime().state() == RuntimeState::Running;
}

void Editor::init() {
    BL_PROFILE_FUNCTION();
    initRenderer();
    createMainWindow();
    initImGui();
    restoreConfigFromDisk();
}

void Editor::initRenderer() {
    BL_PROFILE_FUNCTION();
    auto editorRenderer =
        std::make_unique<EditorRenderer>(makeReceiver(),
                                         coreSystems().takeRenderer());
    editorRenderer->init(device());
    coreSystems().setRenderer(std::move(editorRenderer));
}

struct Editor::Impl {
    struct EditorWindowDelegate: WindowDelegate {
        EditorWindowDelegate(Editor* editor): editor(editor) {}

        void onTextInput(unsigned code) override {
            editor->imguiCtx.onTextInput(code);
        }

        void onResize(int2) override { editor->invalidateView(); }

        void onClose() override { editor->saveConfigToDisk(); }

        void onFocus() override { editor->dockspace.takeFocus(); }

        Editor* editor;
    };
};

void Editor::createMainWindow() {
    BL_PROFILE_FUNCTION();
    auto desc = loadWindowConfig();
    desc.hasToolbarArea = true;
    auto& window = createWindow(desc);
    window.addDelegate(std::make_unique<Impl::EditorWindowDelegate>(this));
    window.createDefaultSwapchain(device());
    window.setCommandQueue(device().createCommandQueue());
    window.setMinSize({ 600, 400 });
    _mainWindow = &window;
}

void Editor::initImGui() {
    BL_PROFILE_FUNCTION();
    ImGuiContextDescription desc;
    desc.iniFilePath = libraryDir() / "imgui.ini";
    imguiCtx.init(*this, desc);
}

void Editor::shutdown() {
    saveConfigToDisk();
    imguiCtx.shutdown();
}

void Editor::frame() {
    /// Dirty hack to make ImGui happy. By posting an empty event we make sure
    /// the next frame is being rendered. Because ImGui relies on continuous
    /// renderering we need to draw a few frames after every interaction to make
    /// sure views really appear etc.
    if (trickleEmptyEventCount-- > 0) {
        postEmptySystemEvent();
    }
    /// If we simulate we want to render continuously
    if (isSimulating()) {
        invalidateView();
    }
    autoConfigSave();
    Appearance::staticInstance().update();
    if (!mainWindow()) {
        return;
    }
    auto& window = *mainWindow();
    imguiCtx.newFrame();
    dockspace.display(window);
    viewManager.display();
    debugViews.display();
    imguiCtx.endFrame();
    imguiCtx.drawFrame();
}

void Editor::menuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Scene...")) {
            // newScene();
        }
        if (ImGui::MenuItem("Save")) {
            saveAll();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Views")) {
        for (auto& entry: ViewRegistry::entries()) {
            if (ImGui::MenuItem(entry.name().c_str())) {
                viewManager.openView(entry);
            }
        }
        ImGui::EndMenu();
    }
    debugViews.menubar();
    if (ImGui::BeginMenu("Profile")) {
        if (ImGui::MenuItem("Profiler")) {
            Logger::Debug("Profiler view not implemented");
        }
        ImGui::EndMenu();
    }
}

bool Editor::onInput(InputEvent e) {
    invalidateView();
    imguiCtx.onInput(e);
    if (viewManager.onInput(e)) {
        return true;
    }
    /// Give to ourself
    cmd.onInput(e);
    return e.handled();
}

std::filesystem::path Editor::configFile() const {
    return libraryDir() / "Poppy.settings";
}

YAML::Node Editor::getStoredConfig() {
    try {
        return YAML::LoadFile(configFile());
    }
    catch (YAML::BadFile const& e) {
        Logger::Warn("Failed to open config file: ", e.what());
        return YAML::Node();
    }
}

void Editor::saveConfigToDisk() {
    /// We load the existing config to not destroy any saved state that is not
    /// currently active
    YAML::Node root = getStoredConfig();
    auto windows = getWindows();
    if (!windows.empty()) {
        root["Window"] = windows.front()->getDescription();
    }
    root["Views"] = viewManager.serialize();
    YAML::Emitter out;
    out << root;
    auto filename = configFile();
    std::fstream file(filename, std::ios::out);
    if (!file) {
        Logger::Error("Failed to open config file: ", filename);
        return;
    }
    file << out.c_str();
}

void Editor::restoreConfigFromDisk() {
    BL_PROFILE_FUNCTION();
    YAML::Node root = getStoredConfig();
    Logger::Warn("Resetting style colors on startup");
    ImGui::StyleColorsDark();
    viewManager.deserialize(root["Views"]);
}

WindowDescription Editor::loadWindowConfig() {
    try {
        return getStoredConfig()["Window"].as<WindowDescription>();
    }
    catch (YAML::Exception const& e) {
        Logger::Trace("Failed to deserialize window state: ", e.what());
        return WindowDescription{
            .size = { 1200, 800 },
            .position = { 400, 400 },
        };
    }
}

void Editor::autoConfigSave() {
    autoConfigSaveTimer -= time().delta;
    if (autoConfigSaveTimer <= 0) {
        saveConfigToDisk();
        autoConfigSaveTimer = saveConfigInterval();
    }
}

void Editor::saveAll() { coreSystems().assetManager().saveAll(); }

void Editor::invalidateView() {
    trickleEmptyEventCount = 3;
    postEmptySystemEvent();
}

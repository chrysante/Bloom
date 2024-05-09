#include "Poppy/Editor/Editor.h"

#include <fstream>

#include <imgui.h>
#include <imgui_internal.h>
#include <range/v3/view.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Application/Window.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Core/Autorelease.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Runtime/SceneSystem.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Core/Timestamp.h"
#include "Poppy/Editor/EntitySelectionManager.h"
#include "Poppy/UI/Appearance.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Dockspace.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/ImGuiHelpers.h"
#include "Poppy/UI/ViewManager.h"
#include "Poppy/UI/ViewRegistry.h"
#include "Poppy/Views/DebugViews.h"

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

struct Editor::Impl {
    bloom::Window* mainWindow = nullptr;
    ImGuiContext imguiCtx;
    Dockspace dockspace;
    ViewManager viewManager;
    CommandPalette cmd;
    DebugViews debugViews;
    EntitySelectionManager selection;
    float autoConfigSaveTimer = 0;
    std::atomic<int> trickleEmptyEventCount = 0;

    Impl(Editor* editor):
        viewManager(editor, editor->makeReceiver()),
        selection(editor->makeReceiver()) {}

    void deleteSelectedEntities();

    struct EditorWindowDelegate;
};

struct Editor::Impl::EditorWindowDelegate: WindowDelegate {
    EditorWindowDelegate(Editor* editor): editor(editor) {}

    void onTextInput(unsigned code) override {
        editor->impl->imguiCtx.onTextInput(code);
    }

    void onResize(int2) override { editor->invalidateUI(); }

    void onClose() override {
        editor->saveConfigToDisk();
        editor->closeAllWindows();
        editor->impl->mainWindow = nullptr;
        editor->impl->imguiCtx.context->Viewports[0]->PlatformHandle = nullptr;
    }

    void onDestroy() override {
        editor->impl->mainWindow = nullptr;
        editor->impl->imguiCtx.context->Viewports[0]->PlatformHandle = nullptr;
    }

    void onFocus() override { editor->impl->dockspace.takeFocus(); }

    Editor* editor;
};

namespace {

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x)      STRINGIFY_IMPL(x)

struct ToolbarBuildTimeStamp: ToolbarItem {
    char const* BuildConfig = STRINGIFY(BUILD_CONFIG);
    char const* Timestamp = buildTimestamp();

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

Editor::Editor(): impl(std::make_unique<Impl>(this)) {
    setRunLoopMode(RunLoopMode::EventDriven);
    // clang-format off
    addCommand({
        "Save", { Key::S, ModFlags::Control }, [this] { saveAll(); }
    });
    auto* simCmd = addCommand({
        "Simulate",
        { Key::R, ModFlags::Control }, [this] { startSimulation(); },
        [this] {
            return !coreSystems().sceneSystem().scenes().empty() && 
                   !isSimulating();
        }
    });
    auto* stopSimCmd = addCommand({
        "Stop Simulation",
        { Key::Period, ModFlags::Control },
        [this] { stopSimulation(); },
        [this] { return isSimulating(); }
    });
    addCommand({
        "Delete Selected Entities",
        { Key::Backspace, ModFlags::Control },
        [this] { impl->deleteSelectedEntities(); }
    });
    impl->dockspace.setLeftToolbar({
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([this] {
                menuBar();
            }),
        ToolbarSpacer(),
        ToolbarIconButton(simCmd, "play"),
        ToolbarIconButton(stopSimCmd, "stop")
    });
    impl->dockspace.setCenterToolbar({
        ToolbarIconButton("bank")
            .onClick([] {
                Logger::Trace("Some Button Pressed");
            }),
        ToolbarSpacer(),
        ToolbarIconButton("cw")
            .onClick([&] {
                Logger::Debug("Reloading shaders is not implemented");
            })
            .tooltip("Reload Shaders")
    });
    impl->dockspace.setRightToolbar({
        ToolbarIconButton("attention-circled")
            .onClick([]{ Logger::Trace("Some Button Clicked"); }),
        ToolbarSpacer{},
        ToolbarBuildTimeStamp(),
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([] {
                ImGui::Text("Placeholder Menu");
            }),
    }); // clang-format on
}

Editor::~Editor() = default;

EntitySelectionManager& Editor::selection() { return impl->selection; }

void Editor::openView(std::string name, std::function<void(View&)> completion) {
    dispatch(DispatchToken::NextFrame, [=, this] {
        auto* view = impl->viewManager.openView(name);
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

void Editor::invalidateUI() {
    impl->trickleEmptyEventCount = 3;
    postEmptySystemEvent();
}

bloom::Window* Editor::mainWindow() const { return impl->mainWindow; }

void Editor::init() {
    BL_PROFILE_FUNCTION();
    createMainWindow();
    initImGui();
    restoreConfigFromDisk();
}

void Editor::createMainWindow() {
    BL_PROFILE_FUNCTION();
    auto desc = loadWindowConfig();
    desc.hasToolbarArea = true;
    desc.autoResizeSwapchain = true;
    auto& window = createWindow(desc);
    window.addDelegate(std::make_unique<Impl::EditorWindowDelegate>(this));
    window.createDefaultSwapchain(device());
    window.setMinSize({ 600, 400 });
    impl->mainWindow = &window;
}

void Editor::initImGui() {
    BL_PROFILE_FUNCTION();
    ImGuiContextDescription desc;
    desc.iniFilePath = libraryDir() / "imgui.ini";
    impl->imguiCtx.init(*this, desc);
}

void Editor::shutdown() {
    saveConfigToDisk();
    impl->imguiCtx.shutdown();
}

void Editor::frame() {
    /// Dirty hack to make ImGui happy. By posting an empty event we make sure
    /// the next frame is being rendered. Because ImGui relies on continuous
    /// renderering we need to draw a few frames after every interaction to make
    /// sure views really appear etc.
    if (impl->trickleEmptyEventCount-- > 0) {
        postEmptySystemEvent();
    }
    /// If we simulate we want to render continuously
    if (isSimulating()) {
        invalidateUI();
    }
    autoConfigSave();
    Appearance::staticInstance().update();
    if (!mainWindow()) {
        return;
    }
    auto& window = *mainWindow();
    impl->imguiCtx.newFrame();
    impl->dockspace.display(window);
    impl->viewManager.display();
    impl->debugViews.display();
    impl->imguiCtx.endFrame();
    impl->imguiCtx.drawFrame();
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
                impl->viewManager.openView(entry);
            }
        }
        ImGui::EndMenu();
    }
    impl->debugViews.menubar();
    if (ImGui::BeginMenu("Profile")) {
        if (ImGui::MenuItem("Profiler")) {
            Logger::Debug("Profiler view not implemented");
        }
        ImGui::EndMenu();
    }
}

bool Editor::onInput(InputEvent e) {
    invalidateUI();
    impl->imguiCtx.onInput(e);
    if (impl->viewManager.onInput(e)) {
        return true;
    }
    /// Give to ourself
    impl->cmd.onInput(e);
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
    root["Views"] = impl->viewManager.serialize();
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
    impl->viewManager.deserialize(root["Views"]);
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
    impl->autoConfigSaveTimer -= time().delta;
    if (impl->autoConfigSaveTimer <= 0) {
        saveConfigToDisk();
        impl->autoConfigSaveTimer = saveConfigInterval();
    }
}

Command const* Editor::addCommand(Command command) {
    return impl->cmd.add(std::move(command));
}

void Editor::saveAll() { coreSystems().assetManager().saveAll(); }

void Editor::Impl::deleteSelectedEntities() {
    for (EntityHandle entity: selection.entities()) {
        entity.scene().deleteEntity(entity);
    }
    selection.clear();
}

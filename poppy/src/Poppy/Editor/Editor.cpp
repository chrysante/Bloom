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

struct EditorWindowDelegate: public WindowDelegate {
    void frame() override {
        if (onFrame) {
            onFrame();
        }
    }

    utl::function<void()> onFrame;
};

static char const* playButton(Editor const* editor) {
    return editor->isSimulating() ? "stop" : "play";
}

Editor::Editor(): mSelection(makeReceiver()) {
    setRunLoopMode(RunLoopMode::EventDriven);
    // clang-format off
    dockspace.setLeftToolbar({
        /// Very annoying hack to prevent the toolbar from rendering under the 
        /// window buttons on MacOS
        ToolbarEmptyItem().width(75),
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([this] {
                menuBar();
            }),
        ToolbarSpacer{},
        ToolbarIconButton(std::bind_front(playButton, this))
            .onClick([&] {
                isSimulating() ? stopSimulation() :
                                 startSimulation();
            })
            .enabled([&] {
                return !coreSystems().sceneSystem().scenes().empty();
            })
    });
    dockspace.setCenterToolbar({
        ToolbarIconButton("bank")
            .onClick([] {
                Logger::Trace("Some Button Pressed");
            }),
        ToolbarSpacer{},
        ToolbarIconButton("cw")
            .onClick([&] {
#if 0
                dispatch(DispatchToken::NextFrame, ReloadShadersCommand{});
#endif
            })
            .tooltip("Reload Shaders (currently disabled due to a bug)")
    });
    dockspace.setRightToolbar({
        ToolbarIconButton("attention-circled")
            .onClick([]{ Logger::Trace("Some Button Clicked"); }),
        ToolbarSpacer{},
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([] {
                ImGui::Text("Placeholder Menu");
            }),
    });
    // clang-format on
}

std::vector<View*> Editor::getAllViews() {
    return views | transform([](auto& view) { return view.get(); }) |
           ranges::to<std::vector>;
}

void Editor::openView(std::string name, std::function<void(View&)> completion) {
    auto const entry = ViewRegistry::get(name);
    if (!entry) {
        Logger::Warn("Could not find View '", name, "'");
        return;
    }
    dispatch(DispatchToken::NextFrame, [=, this] {
        auto& view = createView(*entry, *getWindows().front());
        if (completion) {
            completion(view);
        }
    });
}

void Editor::startSimulation() { coreSystems().runtime().run(); }

void Editor::stopSimulation() { coreSystems().runtime().stop(); }

bool Editor::isSimulating() const {
    return coreSystems().runtime().state() == RuntimeState::Running;
}

void Editor::init() {
    Logger::Trace("Editor::doInit()");
    initRenderer();
    createMainWindow();
    initImGui();
    restoreConfigFromDisk();
    Logger::Trace("Editor::init() done");
}

void Editor::initRenderer() {
    auto editorRenderer =
        std::make_unique<EditorRenderer>(makeReceiver(),
                                         coreSystems().takeRenderer());
    editorRenderer->init(device());
    coreSystems().setRenderer(std::move(editorRenderer));
}

void Editor::createMainWindow() {
    auto delegate = std::make_unique<EditorWindowDelegate>();
    delegate->onFrame = [this, delegate = delegate.get()] {
        imguiCtx.drawFrame(device(), delegate->window());
    };
    auto& window = createWindow(loadWindowConfig(), std::move(delegate));
    window.onInput([this](InputEvent const& e) { this->onInput(e); });
    window.onTextInput([this](unsigned code) { imguiCtx.onTextInput(code); });
    window.onClose([this] { saveConfigToDisk(); });
    window.createDefaultSwapchain(device());
    window.setCommandQueue(device().createCommandQueue());
}

void Editor::initImGui() {
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
    clearClosingViews();
    Appearance::staticInstance().update();
    auto windows = getWindows();
    if (windows.empty()) {
        return;
    }
    auto& window = *windows.front();
    imguiCtx.newFrame(window);
    dockspace.display(window);
    displayViews();
    debugViews.display();
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
        ViewRegistry::forEach([&](std::string_view name, auto const& entry) {
            auto const [desc, factory, _] = entry;
            if (!desc.persistent) {
                return;
            }
            if (ImGui::MenuItem(name.data())) {
                createView(entry, *getWindows().front());
            }
        });

        ImGui::EndMenu();
    }
    debugViews.menubar();
    if (ImGui::BeginMenu("Profile")) {
        if (ImGui::MenuItem("Profiler")) {
            // profileView.open();
        }
        ImGui::EndMenu();
    }
}

void Editor::displayViews() {
    if (auto const itr = std::find_if(views.begin(), views.end(),
                                      [](auto&& v) { return v->maximized(); });
        itr != views.end())
    {
        auto& view = **itr;
        view.doFrame();
        return;
    }
    for (auto& view: views) {
        view->doFrame();
    }
}

void Editor::onInput(InputEvent e) {
    invalidateView();
    imguiCtx.onInput(e);
    e.dispatch<InputEventMask::KeyUp>([&](KeyEvent event) {
        if (event.key == Key::S && test(event.modifierFlags & ModFlags::Super))
        {
            saveAll();
            return true;
        }
        return false;
    });
    if (e.handled()) {
        return;
    }
    auto const focusedView =
        std::find_if(views.begin(), views.end(),
                     [](auto const& v) { return v->focused(); });
    if (focusedView != views.end()) {
        (**focusedView).onInput(e);
        if (e.handled()) {
            return;
        }
    }
    /// Give to all unfocused panels
    for (auto& view: views) {
        if (view->focused()) {
            continue;
        }
        view->onInput(e);
        if (e.handled()) {
            break;
        }
    }
}

std::filesystem::path Editor::configFile() const {
    return libraryDir() / "Poppy.settings";
}

YAML::Node Editor::getStoredConfig() { return YAML::LoadFile(configFile()); }

void Editor::saveConfigToDisk() {
    /// We load the existing config to not destroy any saved state that is not
    /// currently active
    YAML::Node root = getStoredConfig();
    auto windows = getWindows();
    if (!windows.empty()) {
        root["Window"] = windows.front()->getDescription();
    }
    root["Views"] = getViewsConfig();
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
    YAML::Node root = getStoredConfig();
    Logger::Warn("Resetting style colors on startup");
    ImGui::StyleColorsDark();
    loadViews(root["Views"]);
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

YAML::Node Editor::getViewsConfig() {
    YAML::Node node;
    for (auto& view: views) {
        node.push_back(view->doSerialize());
    }
    return node;
}

void Editor::loadViews(YAML::Node const& node) {
    views.clear();
    for (auto viewData: node) {
        auto view = View::doDeserialize(viewData);
        if (view != nullptr) {
            populateView(*view, *getWindows().front());
            view->doInit();
            views.push_back(std::move(view));
        }
    }
}

View& Editor::createView(ViewRegistry::Entry const& entry, Window& window) {
    assert((bool)entry.factory);
    auto view = entry.factory();
    view->mRegisterDescription = entry.description;
    view->desc.pub._name_DONT_CHANGE_ME = entry.name;
    populateView(*view, window);
    view->doInit();
    views.push_back(std::move(view));
    return *views.back();
}

void Editor::populateView(View& view, Window& window) {
    auto& desc = view.desc;
    desc.editor = this;
    desc.window = &window;
    view.assignEmitter(makeEmitter());
    view.assignReceiver(makeReceiver());
}

void Editor::clearClosingViews() {
    for (auto itr = views.begin(); itr != views.end();) {
        auto& view = *itr;
        if (view->shouldClose()) {
            view->doShutdown();
            itr = views.erase(itr);
        }
        else {
            ++itr;
        }
    }
}

void Editor::saveAll() { coreSystems().assetManager().saveAll(); }

void Editor::invalidateView() {
    trickleEmptyEventCount = 3;
    postEmptySystemEvent();
}

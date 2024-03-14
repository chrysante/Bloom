#include "Poppy/Editor/Editor.hpp"

#include <fstream>

#include <imgui.h>

#include "Bloom/Application/ResourceUtil.hpp"
#include "Bloom/Asset/AssetManager.hpp"
#include "Bloom/Core/Autorelease.hpp"
#include "Bloom/GPU/HardwareDevice.hpp"
#include "Bloom/Runtime/SceneSystem.hpp"
#include "Poppy/Core/Debug.hpp"
#include "Poppy/Renderer/EditorRenderer.hpp"
#include "Poppy/UI/Appearance.hpp"

using namespace bloom;
using namespace mtl::short_types;
using namespace poppy;

std::unique_ptr<Application> bloom::createApplication() {
    return std::make_unique<poppy::Editor>();
}

struct EditorWindowDelegate: public bloom::WindowDelegate {
    void frame() override { onFrame(); }

    utl::function<void()> onFrame;
};

Editor::Editor(): mSelection(makeReceiver()) {
    // clang-format off
    dockspace.setLeftToolbar({
        ToolbarDropdownMenu()
            .icon([] { return "menu"; })
            .content([] {
                ImGui::Text("fndsjsnsaf");
            }),
        ToolbarSpacer{},
        ToolbarIconButton([this] {
            return "play";
//            return sceneSystem().isSimulating() ? "stop" : "play";
        })
            .onClick([this] {
//                sceneSystem().isSimulating() ? stopSimulation() :
//                                               startSimulation();
              })
              .enabled([this] {
                  return false;
//                  return !!sceneSystem().getScene();
              }) });

    dockspace.setCenterToolbar({
        ToolbarIconButton("plus").onClick([] {
            Logger::Trace("Some Button Pressed");
        }),
        ToolbarIconButton("bank")
            .onClick([] {
                Logger::Trace("Some Button Pressed");
            }),
        ToolbarSpacer{},
        ToolbarIconButton("cw")
            .onClick([this] {
                dispatch(DispatchToken::NextFrame,
                         ReloadShadersCommand{});
            })
            .tooltip("Reload Shaders")
    });
    dockspace.setRightToolbar({
        ToolbarIconButton("attention-circled"),
        ToolbarSpacer{},
        ToolbarIconButton("menu")
    });
    // clang-format on
}

utl::vector<View*> Editor::getViews() {
    utl::vector<View*> result;
    result.reserve(views.size());
    std::transform(views.begin(), views.end(), std::back_inserter(result),
                   [](auto&& v) { return v.get(); });
    return result;
}

void Editor::openView(std::string name, utl::function<void(View&)> completion) {
    auto const entry = ViewRegistry::get(name);
    if (!entry) {
        Logger::Warn("Could not find View '", name, "'");
        return;
    }

    dispatch(DispatchToken::NextFrame, [=] {
        auto& view = createView(*entry, *getWindows().front());
        if (completion) {
            completion(view);
        }
    });
}

/// MARK: Init
void Editor::init() {
    auto editorRenderer =
        std::make_unique<EditorRenderer>(makeReceiver(),
                                         coreSystems().getRenderer());
    editorRenderer->init(device());
    coreSystems().setRenderer(std::move(editorRenderer));

    /* Create a Window */ {
        WindowDescription windowDesc;
        windowDesc.size = { 1200, 800 };

        auto delegate = std::make_unique<EditorWindowDelegate>();
        delegate->onFrame = [this, delegate = delegate.get()] {
            imguiCtx.drawFrame(device(), delegate->window());
        };
        auto& window = createWindow(windowDesc, std::move(delegate));
        window.onInput([this](InputEvent const& e) { this->onInput(e); });
        window.onCharInput(
            [this](unsigned int code) { imguiCtx.onCharInput(code); });

        window.createDefaultSwapchain(device());
        window.setCommandQueue(device().createCommandQueue());
    }

    /* Initialize ImGui */ {
        ImGuiContextDescription desc;
        desc.iniFilePath = bloom::libraryDir() / "imgui.ini";
        imguiCtx.init(*this, desc);
    }

    loadStateFromDisk();
}

/// MARK: Shutdown
void Editor::shutdown() {
    saveStateToDisk();
    imguiCtx.shutdown();
}

/// MARK: Frame
void Editor::frame() {
    saveStateDirtyTimer -= time().delta;
    if (saveStateDirtyTimer <= 0) {
        saveStateToDisk();
        saveStateDirtyTimer = saveStateInterval;
    }
    clearClosingViews();
    auto const windows = getWindows();
    if (windows.empty()) {
        BL_DEBUGBREAK("Can this happen?");
        return;
    }
    appearance.update();
    imguiCtx.newFrame(*windows.front());
    menuBar();
    dockspace.display();
    displayViews();
    debugViews.display();
}

void Editor::menuBar() {
    ImGui::BeginMainMenuBar();
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

    ImGui::EndMainMenuBar();
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

/// MARK: Serialization

void Editor::onInput(bloom::InputEvent e) {
    imguiCtx.onInput(e);
    e.dispatch<InputEventType::keyUp>([&](KeyEvent event) {
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

/// MARK: Serialization
void Editor::saveStateToDisk() {
    YAML::Node root;
    root["Appearance"] = appearance.serialize();
    root["Views"] = saveViews();
    YAML::Emitter out;
    out << root;
    auto const filename = settingsFile();
    std::fstream file(filename, std::ios::out);
    if (!file) {
        Logger::Error("Failed to save to file: ", filename);
        return;
    }
    file << out.c_str();
}

void Editor::loadStateFromDisk() {
    auto const text = readFileText(settingsFile());
    if (!text) {
        return;
    }
    YAML::Node root = YAML::Load(*text);
    appearance.deserialize(root["Appearance"]);
    Logger::Warn("Resetting style colors on startup");
    ImGui::StyleColorsDark();
    loadViews(root["Views"]);
}

YAML::Node Editor::saveViews() {
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

/// MARK: Misc

std::filesystem::path Editor::settingsFile() const {
    return libraryDir() / "Poppy.settings";
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

void Editor::populateView(View& view, bloom::Window& window) {
    auto& desc = view.desc;
    desc.editor = this;
    desc.window = &window;

    view.Emitter::operator=(makeEmitter());
    view.Receiver::operator=(makeReceiver());
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

void Editor::saveAll() {
    coreSystems().assetManager().saveAll();
    for (auto* scene: coreSystems().sceneSystem().scenes()) {
        coreSystems().assetManager().saveToDisk(scene->handle());
    }
}

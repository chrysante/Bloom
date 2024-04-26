#ifndef POPPY_EDITOR_EDITOR_H
#define POPPY_EDITOR_EDITOR_H

#include "Poppy/Editor/SelectionContext.h"

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include <utl/messenger.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/Window.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Dockspace.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/ViewManager.h"
#include "Poppy/Views/BasicSceneInspector.h"
#include "Poppy/Views/DebugViews.h"

namespace poppy {

class Editor: public bloom::Application {
public:
    Editor();

    /// The entity selection context. This is owned by the editor because
    /// several views edit and read this
    SelectionContext& selection() { return mSelection; }

    /// Opens the view with \p name on the next frame. \p completion will be run
    /// after opening the view
    void openView(std::string name,
                  std::function<void(View&)> completion = nullptr);

    /// \Returns `true` if the game is currently simulated
    bool isSimulating() const;

    /// Request the view to be redrawn. Can be called asynchronously
    void invalidateView();

    /// ImGui expects one window to be the "main" window so we have this here
    bloom::Window* mainWindow() const { return _mainWindow; }

private:
    struct Impl;

    void init() override;
    void initRenderer();
    void createMainWindow();
    void initImGui();
    void shutdown() override;
    void frame() override;

    void menuBar();
    bool onInput(bloom::InputEvent event) override;

    /// # Config serialization
    /// @{
    /// \Returns the path of the config file
    std::filesystem::path configFile() const;
    /// Loads the config file
    YAML::Node getStoredConfig();
    void saveConfigToDisk();
    void restoreConfigFromDisk();
    bloom::WindowDescription loadWindowConfig();
    /// Run every frame to regulary save config
    void autoConfigSave();
    /// @}

    /// Saves all currently open assets
    void saveAll();

    /// Start simulating the world
    void startSimulation();

    /// Stops a running simulation
    void stopSimulation();

    bloom::Window* _mainWindow = nullptr;
    ImGuiContext imguiCtx;
    Dockspace dockspace;
    ViewManager viewManager;
    CommandPalette cmd;
    DebugViews debugViews;
    SelectionContext mSelection;
    float autoConfigSaveTimer = 0;
    std::atomic<int> trickleEmptyEventCount = 0;
};

/// Hack to make ImGui happy, described in the source file
void postEmptySystemEvent();

} // namespace poppy

#endif // POPPY_EDITOR_EDITOR_H

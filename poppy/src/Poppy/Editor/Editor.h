#ifndef POPPY_EDITOR_EDITOR_H
#define POPPY_EDITOR_EDITOR_H

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/Fwd.h"
#include "Poppy/UI/Fwd.h"

namespace poppy {

class EntitySelectionManager;
class ImGuiContext;

class Editor: public bloom::Application {
public:
    Editor();

    ~Editor();

    /// The entity selection manager. This is owned by the editor and shared
    /// between views
    EntitySelectionManager& selection();

    /// Opens the view with \p name on the next frame. \p completion will be run
    /// after opening the view
    void openView(std::string name,
                  std::function<void(View&)> completion = nullptr);

    /// \Returns `true` if the game is currently simulated
    bool isSimulating() const;

    /// Request the UI to be redrawn. Can be called asynchronously
    void invalidateUI(int numFrames = 3);

    /// ImGui expects one window to be the "main" window so we have this here
    bloom::Window* mainWindow() const;

    /// Adds commands to \p commandPalette that modify the open scenes
    void reactToSceneCommands(CommandPalette& commandPalette);

private:
    struct Impl;
    friend class poppy::ImGuiContext; /// To access the command palette for the
                                      /// open test UI command

    void init() override;
    void createMainWindow();
    void initImGui();
    void shutdown() override;
    void frame() override;

    void menuBar();
    bool onInput(bloom::InputEvent event) override;
    Command const* addCommand(Command command);

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

    std::unique_ptr<Impl> impl;
};

/// Hack to make ImGui happy, described in the source file
void postEmptySystemEvent();

} // namespace poppy

#endif // POPPY_EDITOR_EDITOR_H

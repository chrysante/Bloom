#ifndef POPPY_EDITOR_EDITOR_H
#define POPPY_EDITOR_EDITOR_H

#include "Poppy/Editor/SelectionContext.h"

#include <functional>
#include <memory>
#include <vector>

#include <utl/messenger.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/Window.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/Editor/Views/DebugViews.h"
#include "Poppy/UI/Dockspace.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/View.h"

namespace poppy {

class Editor: public bloom::Application {
public:
    Editor();

    /// The entity selection context. This is owned by the editor because
    /// several views edit and read this
    SelectionContext& selection() { return mSelection; }

    /// \Returns a list of all currently open views
    std::vector<View*> getAllViews();

    /// Opens the view with \p name on the next frame. \p completion will be run
    /// after opening the view
    void openView(std::string name,
                  std::function<void(View&)> completion = nullptr);

    /// \Returns `true` if the game is currently simulated
    bool isSimulating() const;

private:
    void init() override;
    void initRenderer();
    void createMainWindow();
    void initImGui();
    void shutdown() override;
    void frame() override;

    void menuBar();
    void displayViews();
    void onInput(bloom::InputEvent event);

    /// # Config serialization
    /// @{

    std::filesystem::path configFile() const;
    /// Loads the config file
    YAML::Node getStoredConfig();
    void saveConfigToDisk();
    void restoreConfigFromDisk();
    bloom::WindowDescription loadWindowConfig();
    /// Run every frame to regulary save config
    void autoConfigSave();
    /// \Returns a YAML node storing the configuration of all open views
    YAML::Node getViewsConfig();
    /// Opens all views from configs stored in \p node
    void loadViews(YAML::Node const& node);

    /// @}

    /// Misc
    View& createView(ViewRegistry::Entry const& entry, bloom::Window& window);
    void populateView(View& view, bloom::Window& window);
    void clearClosingViews();

    /// Saves all currently open assets
    void saveAll();

    void startSimulation();
    void stopSimulation();

    /// Hack to make ImGui happy, described in the source file
    void invalidateView();
    void postEmptySystemEvent();

    ImGuiContext imguiCtx;
    Dockspace dockspace;
    std::vector<std::unique_ptr<View>> views;
    DebugViews debugViews;
    SelectionContext mSelection;
    float autoConfigSaveTimer = 0;
    int trickleEmptyEventCount = 0;
};

} // namespace poppy

#endif // POPPY_EDITOR_EDITOR_H

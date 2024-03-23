#ifndef POPPY_EDITOR_EDITOR_H
#define POPPY_EDITOR_EDITOR_H

#include "Poppy/Editor/SelectionContext.h"

#include <memory>

#include <utl/messenger.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Application/Window.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/Editor/Views/DebugViews.h"
#include "Poppy/UI/Dockspace.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/View.h"

namespace poppy {

/// \Returns the duration in seconds after which the state of the editor is
/// automatically saved
float saveStateInterval();

class Editor: public bloom::Application {
public:
    Editor();

    SelectionContext& selection() { return mSelection; }

    utl::vector<View*> getViews();

    void openView(std::string name,
                  utl::function<void(View&)> completion = nullptr);

    ///
    bool isSimulating() const;

private:
    /// Init
    void init() override;

    /// Shutdown
    void shutdown() override;

    /// Frame
    void frame() override;
    void menuBar();
    void displayViews();

    /// Input
    void onInput(bloom::InputEvent);

    /// Serialization
    std::filesystem::path configFile() const;
    YAML::Node getConfig();
    void saveStateToDisk();
    void loadStateFromDisk();
    bloom::WindowDescription loadWindowDesc();
    YAML::Node saveViews();
    void loadViews(YAML::Node const&);

    /// Misc
    View& createView(ViewRegistry::Entry const&, bloom::Window&);
    void populateView(View&, bloom::Window&);
    void clearClosingViews();

    ///
    void saveAll();

    ///
    void startSimulation();

    ///
    void stopSimulation();

    /// Hack to make ImGui happy, described in the source file
    void invalidateView();
    void postEmptySystemEvent();

    ImGuiContext imguiCtx;
    Dockspace dockspace;
    utl::vector<std::unique_ptr<View>> views;
    DebugViews debugViews;
    SelectionContext mSelection;
    float saveStateDirtyTimer = saveStateInterval();
    int trickleEmptyEventCount = 0;
};

} // namespace poppy

#endif // POPPY_EDITOR_EDITOR_H

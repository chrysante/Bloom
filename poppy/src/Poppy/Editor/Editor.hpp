#include "Poppy/Editor/SelectionContext.hpp"

#include <memory>

#include <utl/messenger.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application/Application.hpp"
#include "Bloom/Application/Window.hpp"
#include "Poppy/Editor/Views/DebugViews.hpp"
#include "Poppy/UI/Dockspace.hpp"
#include "Poppy/UI/ImGuiContext.hpp"
#include "Poppy/UI/View.hpp"

namespace poppy {

float const saveStateInterval = 5; // in seconds

class Editor: public bloom::Application {
public:
    Editor();

    SelectionContext& selection() { return mSelection; }

    utl::vector<View*> getViews();
    void openView(std::string name,
                  utl::function<void(View&)> completion = nullptr);

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
    void saveStateToDisk();
    void loadStateFromDisk();
    YAML::Node saveViews();
    void loadViews(YAML::Node const&);

    /// Misc
    std::filesystem::path settingsFile() const;
    View& createView(ViewRegistry::Entry const&, bloom::Window&);
    void populateView(View&, bloom::Window&);
    void clearClosingViews();

    ///
    void saveAll();

private:
    ImGuiContext imguiCtx;
    Dockspace dockspace;
    utl::vector<std::unique_ptr<View>> views;
    DebugViews debugViews;
    SelectionContext mSelection;
    float saveStateDirtyTimer = saveStateInterval;
};

} // namespace poppy

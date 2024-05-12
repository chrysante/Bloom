#ifndef POPPY_UI_VIEW_H
#define POPPY_UI_VIEW_H

#include <optional>
#include <string>

#include <utl/hashmap.hpp>
#include <utl/messenger.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/Fwd.h"
#include "Bloom/Application/MessageSystem.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Toolbar.h"

namespace poppy {

class Editor;

/// View constructor arguments
struct ViewDescription {
    /// The default display size of the view
    vml::int2 defaultSize = { 500, 400 };

    /// The minimum display size of the view
    vml::int2 minSize = { 300, 200 };
};

/// Base class of all UI views
///
/// Wraps an ImGui window and extends and specializes its functionality
class View: protected bloom::Emitter, protected bloom::Receiver {
public:
    virtual ~View() = default;

    /// \Returns `true` if this view is the focused view
    bool focused() const;

    /// \Returns `true` if this view is maximized
    bool maximized() const { return data.maximized; }

    /// \Returns `true` if the user requested to close this view
    bool shouldClose() const { return !data.open; }

    /// \Returns the size of this view's window.
    vml::int2 size() const { return data.size; }

    /// \Returns the position of this view in screen coordinates
    vml::int2 position() const { return data.position; }

    /// \Returns the padding of this view
    vml::float2 padding() const;

    /// \Returns the title of this view
    std::string const& title() const { return data.title; }

    /// \Returns the static name of this view
    std::string const& staticName() const { return data.staticName; }

    /// \Returns the editor instance
    Editor& editor() const { return *data.editor; }

    /// \Returns the window in which this view resides
    bloom::Window& window() const;

    /// Focuses this view
    void setFocused();

    /// Maximizes this view
    void maximize();

    /// Restores this view
    void restore();

    /// Maximizes the view if restores and vice versa
    void toggleMaximize();

    /// Sets the window title to \p title
    void setTitle(std::string title) { data.title = std::move(title); }

    /// Sets the window padding to \p padding
    /// If \p padding is unengaged `ImGui::Style().WindowPadding` will be used
    void setPadding(vml::vector<std::optional<float>, 2> padding) {
        data.padding = padding;
    }

    /// Sets the horizontal window padding
    void setPaddingX(std::optional<float> value) { data.padding.x = value; }

    /// Sets the vertical window padding
    void setPaddingY(std::optional<float> value) { data.padding.y = value; }

    /// Sets the window background color to \p color
    /// If \p color is unengaged `ImGuiCol_WindowBG` will be used
    void setBGColor(std::optional<vml::float4> color) { data.bgColor = color; }

    /// \Returns the attached command palette
    CommandPalette const& commandPalette() const { return cmdPalette; }

    /// Converts the point \p p from screen coordinates into view coordinates
    vml::float2 screenSpaceToViewSpace(vml::float2 p) const;

    /// Converts the point \p p from view coordinates into screen coordinates
    vml::float2 viewSpaceToScreenSpace(vml::float2 p) const;

    /// Can be used to send data to derived views without knowing their type.
    /// Unexpected data will be ignored
    void openData(std::any data) { doOpenData(std::move(data)); }

    struct BackendImpl;

protected:
    explicit View(ViewDescription desc = {});

    /// \Returns a mutable reference to the attached command palette
    CommandPalette& commandPaletteWrite() { return cmdPalette; }

    /// Calls `display()` on the toolbar of this view. This is virtual to allow
    /// customized toolbar rendering
    virtual void displayToolbar();

    /// \Returns a pointer this view's toolbar. This will be null if no toolbar
    /// is set
    Toolbar const* toolbar() const { return _toolbar ? &*_toolbar : nullptr; }

    /// Sets the toolbar of this view
    void setToolbar(Toolbar toolbar) { _toolbar = std::move(toolbar); }

private:
    /// Called after the view has been created and potentially deserialized
    virtual void init() {};

    /// Called on every frame the application is rendered
    virtual void display() {};

    /// Called for all input events
    virtual void onInput(bloom::InputEvent&) {}

    /// Callback to serialize derived view data
    virtual YAML::Node serialize() const { return YAML::Node{}; }

    /// Callback to deserialize derived view data
    virtual void deserialize(YAML::Node) {}

    /// Callback to receive data
    virtual void doOpenData(std::any){};

    friend class ViewRegistryEntry;
    friend class ViewManager;

    void doInit();
    void doFrame();
    void doOnInput(bloom::InputEvent& event);
    YAML::Node doSerialize() const;
    static std::unique_ptr<View> doDeserialize(YAML::Node const& node);

    struct Data {
        /// This is set by `doInit()` to a new view ID if it is -1 and by
        /// `doDeserialize()` if the view is not newly created
        int ID = -1;
        /// Name of the derived view class. Used for serialization
        std::string staticName;

        vml::float2 position;
        vml::float2 size;
        vml::float2 minSize;
        bool docked = false;
        std::string title;
        void* imguiWindow = nullptr;
        Editor* editor = nullptr;
        bloom::Window* window = nullptr;
        vml::vector<std::optional<float>, 2> padding;
        std::optional<vml::float4> bgColor;
        bool maximized = false;
        bool open = true;
        bool isFocused = false;
        /// `ImGuiWindowFlags` to use on the next frame. This is reset after the
        /// call to `Begin()`
        int nextFrameFlags = 0;
    };
    struct InputState {
        bool hovered = false;
        bool capture = false;
    };
    Data data;
    InputState inputState;
    CommandPalette cmdPalette;
    std::optional<Toolbar> _toolbar;
};

} // namespace poppy

#endif // POPPY_UI_VIEW_H

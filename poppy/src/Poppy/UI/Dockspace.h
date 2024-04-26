#ifndef POPPY_UI_DOCKSPACE_H
#define POPPY_UI_DOCKSPACE_H

#include <span>

#include <vml/vml.hpp>

#include "Poppy/UI/Toolbar.h"

namespace bloom {

class Window;

} // namespace bloom

namespace poppy {

/// Wrapper around ImGui's docking feature
class Dockspace {
public:
    /// Makes the entire content area of \p window a dockable area and draws
    /// toolbars in the windows titlebar
    void display(bloom::Window& window);

    /// # Toolbar Layout
    /// A dockspace has three toolbars. Depending on the docked windows in the
    /// dockspace a layout is for the three toolbars is automatically chosen.
    /// For example if there is a window on the left and on the right side of
    /// the dock space which are not larger than one third of the dockspace, the
    /// toolbars are aligned to these windows:
    /// ```
    /// ┌──────────────┬──────────────────┬───────────────┐
    /// │ Left Toolbar │ Center Toolbar   │ Right Toolbar │
    /// ├──────────────┼──────────────────┼───────────────┤
    /// │              │                  │               │
    /// │ Left Window  │ Center Window(s) │ Right Window  │
    /// │              │                  │               │
    /// └──────────────┴──────────────────┴───────────────┘
    /// ┌──────────────┬──────────────────┬───────────────┐
    /// │ Left Toolbar │ Center Toolbar   │ Right Toolbar │
    /// ├──────────────┼──────────────────┴───────────────┤
    /// │              │                                  │
    /// │ Left Window  │        Other Windows             │
    /// │              │                                  │
    /// └──────────────┴──────────────────────────────────┘
    /// ┌──────────────┬──────────────────┬───────────────┐
    /// │ Left Toolbar │ Center Toolbar   │ Right Toolbar │
    /// ├──────────────┴──────────────────┼───────────────┤
    /// │                                 │               │
    /// │          Left Windows           │ Right Window  │
    /// │                                 │               │
    /// └─────────────────────────────────┴───────────────┘
    /// ┌──────────────┬──────────────────┬───────────────┐
    /// │ Left Toolbar │ Center Toolbar   │ Right Toolbar │
    /// ├──────────────┴──────────────────┴───────────────┤
    /// │                                                 │
    /// │ Single window or windows that don't meet the    │
    /// │ requirements                                    │
    /// │                                                 │
    /// └─────────────────────────────────────────────────┘
    /// ```

    /// Assigns the left toolbar
    void setLeftToolbar(Toolbar toolbar);

    /// Assigns the center toolbar
    void setCenterToolbar(Toolbar toolbar);

    /// Assigns the right toolbar
    void setRightToolbar(Toolbar toolbar);

    /// Focuses the last focused child window in the dockspace
    void takeFocus();

private:
    void mainWindow();

    void displayToolbars(bloom::Window& window);

    vml::float2 minWindowSize = { 250, 100 };
    std::array<Toolbar, 3> toolbars;

    void* mainImGuiWindow = nullptr;
    size_t spacingCount = 0;
    unsigned mainDockID = 0;
};

} // namespace poppy

#endif // POPPY_UI_DOCKSPACE_H

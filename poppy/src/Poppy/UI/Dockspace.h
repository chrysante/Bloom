#ifndef POPPY_UI_DOCKSPACE_H
#define POPPY_UI_DOCKSPACE_H

#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Poppy/UI/Toolbar.h"

namespace bloom {

class Window;

} // namespace bloom

namespace poppy {

class Dockspace {
public:
    Dockspace();

    void display(bloom::Window& window);

    void setLeftToolbar(Toolbar toolbar);
    void setCenterToolbar(Toolbar toolbar);
    void setRightToolbar(Toolbar toolbar);

    /// Zones from left and right window border where no controls should be
    /// displayed
    void setInsets(float left, float right);

private:
    void mainWindow();

    void submitMasterDockspace();

    void displayToolbar(bloom::Window& window);

    void toolbarLayoutOne(utl::small_vector<int, 2>);
    void toolbarLayoutTwo(utl::small_vector<int, 2>);
    void toolbarLayoutThree(utl::small_vector<int, 2>);

    utl::small_vector<int, 2> getToolbarSpacing() const;
    void setToolbarHeight(float height);

private:
    vml::float2 minWindowSize = { 250, 100 };
    std::array<Toolbar, 3> toolbars;
    float leftInset = 0, rightInset = 0;

    std::size_t spacingCount = 0;
    unsigned mainDockID = 0;
};

} // namespace poppy

#endif // POPPY_UI_DOCKSPACE_H

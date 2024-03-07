#pragma once

#include <mtl/mtl.hpp>
#include <utl/vector.hpp>

#include "Poppy/UI/Toolbar.hpp"

namespace poppy {

class Dockspace {
public:
    Dockspace();
    void display();

    void setLeftToolbar(Toolbar);
    void setCenterToolbar(Toolbar);
    void setRightToolbar(Toolbar);

private:
    void dockspace();
    void mainWindow();

    void submitMasterDockspace();

    void displayToolbar();

    void toolbarLayoutOne(utl::small_vector<int, 2>);
    void toolbarLayoutTwo(utl::small_vector<int, 2>);
    void toolbarLayoutThree(utl::small_vector<int, 2>);

    utl::small_vector<int, 2> getToolbarSpacing() const;

private:
    mtl::float2 minWindowSize = { 250, 100 };
    std::array<Toolbar, 3> toolbars;

    std::size_t spacingCount = 0;
    unsigned mainDockID = 0;
};

} // namespace poppy

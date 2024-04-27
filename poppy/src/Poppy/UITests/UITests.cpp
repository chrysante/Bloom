#include "Poppy/UI/TestRegistry.h"
#include "Poppy/UI/ViewManager.h"

using namespace poppy;
using namespace bloom;

UI_TEST("General", "Example Test") {
    openView("Content Browser");
    openView("Viewport");
    context().Yield();
    context().SetRef(context().WindowInfo("Content Browser/Directory View").ID);
    context().ItemDoubleClick("###MyScene");
}

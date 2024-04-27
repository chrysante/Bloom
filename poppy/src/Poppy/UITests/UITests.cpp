#include "Poppy/UI/TestRegistry.h"
#include "Poppy/UI/ViewManager.h"

using namespace poppy;
using namespace bloom;

UI_TEST("General", "Example Test", ImGuiTestContext* ctx, Emitter& emitter) {
    emitter.dispatch(DispatchToken::Now,
                     ViewManager::OpenCommand{ "Content Browser" });
    emitter.dispatch(DispatchToken::Now,
                     ViewManager::OpenCommand{ "Viewport" });
    ctx->Yield();
    ctx->SetRef(ctx->WindowInfo("Content Browser/Directory View").ID);
    ctx->ItemDoubleClick("###MyScene");
};

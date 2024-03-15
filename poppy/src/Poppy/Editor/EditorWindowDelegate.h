#ifndef POPPY_EDITOR_EDITORWINDOWDELEGATE_H
#define POPPY_EDITOR_EDITORWINDOWDELEGATE_H

#include <utl/messenger.hpp>
#include <utl/vector.hpp>

#include "Bloom/Application.h"
#include "Poppy/UI/Dockspace.h"
#include "Poppy/UI/ImGuiContext.h"
#include "Poppy/UI/View.h"

namespace poppy {

class EditorWindowDelegate: public bloom::WindowDelegate {
private:
    void init() override;
    void shutdown() override;
    void frame() override;

private:
    void loadViews();

    void populateView(View&);

    void clearClosingViews();

private:
    ImGuiContext imguiCtx;
    Dockspace dockspace;
    utl::vector<std::unique_ptr<View>> views;
};

} // namespace poppy

#endif // POPPY_EDITOR_EDITORWINDOWDELEGATE_H

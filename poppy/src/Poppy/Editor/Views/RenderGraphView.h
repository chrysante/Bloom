#ifndef POPPY_EDITOR_VIEWS_RENDERGRAPHVIEW_H
#define POPPY_EDITOR_VIEWS_RENDERGRAPHVIEW_H

#include "Bloom/Application/InputEvent.h"
#include "Poppy/Editor/Views/NodeEditor.h"
#include "Poppy/UI/View.h"

namespace poppy {

class RenderGraphView: public View {
public:
    void init() override;

    void onInput(bloom::InputEvent& event) override;

    void frame() override;

private:
    NodeEditor nodeEditor;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_RENDERGRAPHVIEW_H

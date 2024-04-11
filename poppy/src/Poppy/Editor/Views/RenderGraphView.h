#ifndef POPPY_EDITOR_VIEWS_RENDERGRAPHVIEW_H
#define POPPY_EDITOR_VIEWS_RENDERGRAPHVIEW_H

#include "Bloom/Application/InputEvent.h"
#include "Poppy/Editor/Views/BasicSceneInspector.h"
#include "Poppy/Editor/Views/NodeEditor.h"
#include "Poppy/UI/View.h"

namespace bloom::rg {

class RenderGraph;

} // namespace bloom::rg

namespace poppy {

class RenderGraphView: public View {
public:
    RenderGraphView();

    void init() override;

    void onInput(bloom::InputEvent& event) override;

    void frame() override;

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

private:
    NodeEditor nodeEditor;
    bloom::rg::RenderGraph* renderGraph;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_RENDERGRAPHVIEW_H

#ifndef POPPY_EDITOR_VIEWS_RENDERERSETTINGSVIEW_H
#define POPPY_EDITOR_VIEWS_RENDERERSETTINGSVIEW_H

#include "Bloom/Graphics/Renderer.h"
#include "Poppy/UI/View.h"
#include "PropertiesView.hpp"

namespace poppy {

class RendererSettingsView: public View, PropertiesView {
public:
    RendererSettingsView();

private:
    void frame() override;

    void shadowProperties();
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_RENDERERSETTINGSVIEW_H

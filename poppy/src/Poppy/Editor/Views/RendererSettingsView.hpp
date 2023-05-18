#pragma once

#include "Bloom/Graphics/Renderer.hpp"
#include "Poppy/UI/View.hpp"
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

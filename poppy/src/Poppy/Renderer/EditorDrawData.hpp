#pragma once

#include "Bloom/Graphics/ShaderBase.hpp"

namespace poppy {

struct OverlayDrawDescription {
    bool drawSelection = true;
    float selectionLineWidth = 4;
    metal::float4 selectionLineColor = { 1, 0.5, 0, 1 };
};

struct EditorDrawData {
    OverlayDrawDescription overlayDrawDesc;
};

} // namespace poppy

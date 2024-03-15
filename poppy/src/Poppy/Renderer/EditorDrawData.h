#ifndef POPPY_RENDERER_EDITORDRAWDATA_H
#define POPPY_RENDERER_EDITORDRAWDATA_H

#include "Bloom/Graphics/ShaderBase.h"

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

#endif // POPPY_RENDERER_EDITORDRAWDATA_H

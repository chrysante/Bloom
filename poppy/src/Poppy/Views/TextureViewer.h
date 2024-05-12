#ifndef POPPY_VIEWS_TEXTUREVIEWER_H
#define POPPY_VIEWS_TEXTUREVIEWER_H

#include "Bloom/Asset/Fwd.h"

namespace poppy {

void previewTexture(bloom::Texture2D const& texture, float rounding = 5);

} // namespace poppy

#endif // POPPY_VIEWS_TEXTUREVIEWER_H

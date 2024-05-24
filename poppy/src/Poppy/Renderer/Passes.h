#ifndef POPPY_RENDERER_PASSES_H
#define POPPY_RENDERER_PASSES_H

#include "Bloom/Graphics/RenderGraph/Fwd.h"

namespace poppy {

std::unique_ptr<bloom::Pass> makeSelectionPass();
std::unique_ptr<bloom::Pass> makeCompositionPass();
std::unique_ptr<bloom::Pass> makeOverlayPass();

} // namespace poppy

#endif // POPPY_RENDERER_PASSES_H

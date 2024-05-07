#ifndef POPPY_RENDERER_PASSES_H
#define POPPY_RENDERER_PASSES_H

#include "Bloom/Graphics/RenderGraph/Fwd.h"

namespace poppy {

std::unique_ptr<bloom::rg::Pass> makeSelectionPass();
std::unique_ptr<bloom::rg::Pass> makeCompositionPass();
std::unique_ptr<bloom::rg::Pass> makeGridPass();

} // namespace poppy

#endif // POPPY_RENDERER_PASSES_H

#include "Bloom/Graphics/RenderGraph/ShadowPass.h"

using namespace bloom;

RGShadowPass::RGShadowPass(): RGPassNode("Shadow Pass") {
    declareInput(RGBufferDescription{
        .name = "Geometry",
    });
    declareInput(RGBufferDescription{
        .name = "Lights",
    });
}

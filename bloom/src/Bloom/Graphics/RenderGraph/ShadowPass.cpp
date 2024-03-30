#include "Bloom/Graphics/RenderGraph/ShadowPass.h"

using namespace bloom;
using namespace rg;

RGShadowPass::RGShadowPass(): Pass("Shadow Pass") {
    declareInput(BufferInterface{
        .name = "Geometry",
    });
    declareInput(BufferInterface{
        .name = "Lights",
    });
}

#include "Bloom/Graphics/RenderGraph/ForwardShadingPass.h"

using namespace bloom;

ForwardShadingPass::ForwardShadingPass(): RGPassNode("Forward Shading Pass") {
    declareInput(RGBufferDescription{ .name = "Geometry" });
}

#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Node.h"

namespace bloom {

///
class ForwardShadingPass: public RGPassNode {
public:
    ForwardShadingPass();

private:
    void doExecute(RenderContext& ctx) override;

    // Inputs
    BufferHandle sceneParameters;
    RenderScene* scene;

    // Outputs
    TextureHandle rawColor;
    TextureHandle depth;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

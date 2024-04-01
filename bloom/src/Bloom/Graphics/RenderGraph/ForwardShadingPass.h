#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/ResourceNodes.h"

namespace bloom::rg {

///
class ForwardShadingPass: public Pass {
public:
    explicit ForwardShadingPass(std::string name);

private:
    void doExecute(RenderContext& ctx) override;

    // Inputs
    BufferHandle sceneParameters;
    GeometryResource* geometry;
    LightsResource* lights;

    // Outputs
    TextureHandle rawColor;
    TextureHandle depth;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

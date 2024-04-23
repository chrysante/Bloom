#ifndef BLOOM_GRAPHICS_RENDERGRAPH_SHADOWPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_SHADOWPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"

namespace bloom::rg {

class ShadowPass: public RenderPass, public PassImpl<ShadowPass> {
public:
    ShadowPass(std::string name);

private:
    friend class PassImpl;

    void doExecute(FrameParameters const& params, RenderContext& ctx) override;
    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    // Sinks
    GeometryResource* geometry;
    BufferHandle lightSpaceTransforms; // Buffer of light space "cameras"

    // Sources
    TextureHandle shadowMaps; // Texture array, one element for each cascade

    RenderPipelineHandle pipeline;
    DepthStencilHandle depthStencil;
    SamplerHandle sampler;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_SHADOWPASS_H

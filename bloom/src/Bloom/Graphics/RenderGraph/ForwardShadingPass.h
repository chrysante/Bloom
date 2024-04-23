#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"

namespace bloom::rg {

///
class ForwardShadingPass:
    public RenderPass,
    public PassImpl<ForwardShadingPass> {
public:
    explicit ForwardShadingPass(std::string name);

private:
    friend class PassImpl;

    void doExecute(FrameParameters const& params, RenderContext& ctx) override;
    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    // Sinks
    GeometryResource* geometry;
    LightsResource* lights;
    BufferHandle cameraBuffer;
    BufferHandle lightSpaceTransforms;
    BufferHandle shadowMetadata;
    TextureHandle shadowMaps;

    // Sources
    TextureHandle rawColor;
    TextureHandle depth;

    // Internal resources
    SamplerHandle shadowMapSampler;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

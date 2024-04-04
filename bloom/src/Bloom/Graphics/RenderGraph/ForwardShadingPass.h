#ifndef BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"
#include "Bloom/Graphics/RenderGraph/Resource.h"

namespace bloom::rg {

///
class ForwardShadingPass: public RenderPass {
public:
    explicit ForwardShadingPass(std::string name);

private:
    void doExecute(RenderContext& ctx) override;

    // Inputs
    BufferHandle sceneParameters;
    GeometryResource* geometry = nullptr;
    LightsResource* lights = nullptr;

    // Outputs
    TextureHandle rawColor;
    TextureHandle depth;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_FORWARDSHADINGPASS_H

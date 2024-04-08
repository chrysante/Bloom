#ifndef BLOOM_GRAPHICS_RENDERGRAPH_POSTPROCESSPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_POSTPROCESSPASS_H

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"

namespace bloom::rg {

class PostProcessPass: public ComputePass {
public:
    PostProcessPass(std::string name);

private:
    void doExecute(ComputeContext& ctx) override;
    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    ComputePipelineHandle pipeline;
    BufferHandle parameterBuffer;
    BufferHandle bloomParameterBuffer;
    TextureHandle rawColor;
    TextureHandle bloomColor;
    TextureHandle output;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_POSTPROCESSPASS_H

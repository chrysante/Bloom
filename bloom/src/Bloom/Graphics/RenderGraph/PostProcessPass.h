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
    void doAllocateResources(HardwareDevice& device) override;

    ComputePipelineHandle pipeline;
    BufferHandle parameterBuffer;
    TextureHandle rawColor;
    TextureHandle output;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_POSTPROCESSPASS_H

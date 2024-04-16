#ifndef BLOOM_GRAPHICS_RENDERGRAPH_BLOOMPASS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_BLOOMPASS_H

#include <array>

#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/RenderGraph/Pass.h"

namespace bloom::rg {

class BloomPass: public ComputePass, PassImpl<BloomPass> {
public:
    static constexpr size_t NumMips = 8;

    explicit BloomPass(std::string name);

private:
    friend class PassImpl;

    void doExecute(FrameParameters const& params, ComputeContext& ctx) override;

    void doAllocateResources(FrameParameters const& params,
                             HardwareDevice& device) override;

    ComputePipelineHandle prefilterPipeline;
    ComputePipelineHandle downsamplePipeline;
    ComputePipelineHandle upsamplePipeline;
    ComputePipelineHandle compositePipeline;

    // Sinks
    BufferHandle parameters;
    TextureHandle rawColor;

    // Sources
    TextureHandle output;

    TextureHandle downsample;
    std::array<TextureHandle, NumMips> downsampleMips;
    TextureHandle upsample;
    std::array<TextureHandle, NumMips - 1> upsampleMips;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_BLOOMPASS_H

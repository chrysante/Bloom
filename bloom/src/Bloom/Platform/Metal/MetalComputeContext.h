#ifndef BLOOM_PLATFORM_METAL_METALCOMPUTECONTEXT_H
#define BLOOM_PLATFORM_METAL_METALCOMPUTECONTEXT_H

#include <Metal/Metal.h>

#include "Bloom/GPU/ComputeContext.h"

namespace bloom {

class MetalComputeContext: public ComputeContext {
public:
    MetalComputeContext(id<MTLCommandBuffer>);

    void begin() override;
    void end() override;

    void setPipeline(ComputePipelineView) override;

    void setBuffer(BufferView, std::size_t index,
                   std::size_t offset = 0) override;
    void setBufferOffset(std::size_t index, std::size_t offset) override;
    void setSampler(SamplerView, std::size_t index) override;
    void setSampler(SamplerView, float lodMinClamp, float lodMaxClamp,
                    std::size_t index) override;
    void setTexture(TextureView, std::size_t index) override;

    void dispatchThreads(vml::usize3 threadsPerGrid,
                         vml::usize3 threadsPerThreadgroup) override;

    void commit() override;

    id<MTLCommandBuffer> commandBuffer;
    id<MTLComputeCommandEncoder> commandEncoder;
};

} // namespace bloom

#endif // BLOOM_PLATFORM_METAL_METALCOMPUTECONTEXT_H

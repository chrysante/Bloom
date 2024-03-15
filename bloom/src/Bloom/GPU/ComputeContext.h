#ifndef BLOOM_GPU_COMPUTECONTEXT_H
#define BLOOM_GPU_COMPUTECONTEXT_H

#include "Bloom/Core/Core.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

class ComputeContext {
public:
    virtual ~ComputeContext() = default;

    virtual void begin() = 0;
    virtual void end() = 0;

    virtual void setPipeline(ComputePipelineView) = 0;

    virtual void setBuffer(BufferView, std::size_t index,
                           std::size_t offset = 0) = 0;
    virtual void setBufferOffset(std::size_t index, std::size_t offset) = 0;
    virtual void setSampler(SamplerView, std::size_t index) = 0;
    virtual void setSampler(SamplerView, float lodMinClamp, float lodMaxClamp,
                            std::size_t index) = 0;
    virtual void setTexture(TextureView, std::size_t index) = 0;

    void dispatchThreads(mtl::usize2 threadsPerGrid,
                         mtl::usize2 threadsPerThreadgroup) {
        dispatchThreads({ threadsPerGrid, 1 }, { threadsPerThreadgroup, 1 });
    }
    virtual void dispatchThreads(mtl::usize3 threadsPerGrid,
                                 mtl::usize3 threadsPerThreadgroup) = 0;

    virtual void commit() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_COMPUTECONTEXT_H

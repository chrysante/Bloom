#pragma once

#include <Metal/Metal.h>

#include "Bloom/GPU/RenderContext.hpp"

namespace bloom {

class BLOOM_API MetalRenderContext: public RenderContext {
public:
    MetalRenderContext(id<MTLCommandBuffer>);

    void begin(RenderPassDescription const&) override;
    void end() override;

    void setPipeline(RenderPipelineView) override;
    void setDepthStencil(DepthStencilView) override;

    void setVertexBuffer(BufferView,
                         std::size_t index,
                         std::size_t offset) override;
    void setVertexBufferOffset(std::size_t index, std::size_t offset) override;
    void setVertexTexture(TextureView, std::size_t index) override;
    void setVertexSampler(SamplerView, std::size_t index) override;

    void setFragmentBuffer(BufferView,
                           std::size_t index,
                           std::size_t offset) override;
    void setFragmentBufferOffset(std::size_t index,
                                 std::size_t offset) override;
    void setFragmentTexture(TextureView, std::size_t index) override;
    void setFragmentSampler(SamplerView, std::size_t index) override;

    void setTriangleFillMode(TriangleFillMode) override;
    void setTriangleCullMode(TriangleCullMode) override;

    void draw(DrawDescription const&) override;
    void draw(std::size_t vertexStart, std::size_t vertexCount) override;

    void present(Backbuffer&) override;

    void commit() override;

    id<MTLCommandBuffer> commandBuffer;
    id<MTLRenderCommandEncoder> commandEncoder;
};

} // namespace bloom

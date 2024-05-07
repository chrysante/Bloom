#ifndef BLOOM_GPU_RENDERCONTEXT_H
#define BLOOM_GPU_RENDERCONTEXT_H

#include <utl/vector.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/GPU/CommandDescriptors.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

/// Encodes a sequence of GPU rendering commands
class BLOOM_API RenderContext {
public:
    virtual ~RenderContext() = default;

    /// Must be called before encoding any commands
    virtual void begin(RenderPassDescription const& desc) = 0;

    /// Must be called after encoding all commands
    virtual void end() = 0;

    /// Binds the pipeline state to be used for the next draw call
    virtual void setPipeline(RenderPipelineView pipeline) = 0;

    /// Binds the depth stencil to be used for the next draw call
    virtual void setDepthStencil(DepthStencilView) = 0;

    /// Binds the buffer \p buffer to index \p index for the vertex pipeline
    /// \Param offset Specifies an offset into \p buffer
    virtual void setVertexBuffer(BufferView buffer, std::size_t index,
                                 std::size_t offset = 0) = 0;

    /// Specifies an offset for an already bound vertex buffer
    virtual void setVertexBufferOffset(std::size_t index,
                                       std::size_t offset) = 0;

    /// Binds the texture \p texture to the index \p index for the vertex
    /// pipeline
    virtual void setVertexTexture(TextureView texture, std::size_t index) = 0;

    /// Binds the texture sampler \p sampler to the index \p index for the
    /// vertex pipeline
    virtual void setVertexSampler(SamplerView sampler, std::size_t index) = 0;

    /// Binds the buffer \p buffer to index \p index for the fragment pipeline
    virtual void setFragmentBuffer(BufferView buffer, std::size_t index,
                                   std::size_t offset = 0) = 0;

    /// Specifies an offset for an already bound fragment buffer
    virtual void setFragmentBufferOffset(std::size_t index,
                                         std::size_t offset) = 0;

    /// Binds the texture \p texture to the index \p index for the fragment
    /// pipeline
    virtual void setFragmentTexture(TextureView, std::size_t index) = 0;

    /// Binds the texture sampler \p sampler to the index \p index for the
    /// vertex pipeline
    virtual void setFragmentSampler(SamplerView, std::size_t index) = 0;

    /// Sets the triangle fill mode for the next draw call
    virtual void setTriangleFillMode(TriangleFillMode mode) = 0;

    /// Sets the triangle cull mode for the next draw call
    virtual void setTriangleCullMode(TriangleCullMode mode) = 0;

    /// Encodes an indexed draw call
    virtual void draw(DrawDescription const& desc) = 0;

    /// Encodes a draw call without index buffer
    virtual void draw(std::size_t vertexStart, std::size_t vertexCount) = 0;

    /// Presents \p backbuffer
    virtual void present(Backbuffer& backbuffer) = 0;

    /// Commits the encoded render commands for execution
    virtual void commit() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_RENDERCONTEXT_H

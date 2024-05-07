#ifndef BLOOM_GPU_COMMANDBUFFER_H
#define BLOOM_GPU_COMMANDBUFFER_H

#include "Bloom/GPU/CommandDescriptors.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

/// Stores commands to be executed on the GPU
class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    /// Enqueues this command buffer into its command queue
    virtual void enqueue() = 0;

    /// Commits this command buffer to its command queue and executes it as soos
    /// as possible
    virtual void commit() = 0;

    /// Blocks the calling thread until the command buffer is done executing
    virtual void waitForCompletion() = 0;

    /// Creates a render encoder that encodes commands to this buffer
    virtual std::unique_ptr<RenderEncoder> makeRenderEncoder(
        RenderPassDescription const& desc) = 0;

    /// Creates a compute encoder that encodes commands to this buffer
    virtual std::unique_ptr<ComputeEncoder> makeComputeEncoder() = 0;

    /// Creates a blit encoder that encodes commands to this buffer
    virtual std::unique_ptr<BlitEncoder> makeBlitEncoder() = 0;

protected:
    CommandBuffer() = default;
};

/// Base class of all command encoders
class CommandEncoder {
public:
    virtual ~CommandEncoder() = default;

    /// Must be called after encoding all commands
    virtual void end() = 0;

protected:
    CommandEncoder() = default;
};

/// Encodes render commands to a command buffer
class RenderEncoder: public CommandEncoder {
public:
    /// Binds the pipeline state to be used for the next draw call
    virtual void setPipeline(RenderPipelineView pipeline) = 0;

    /// Binds the depth stencil to be used for the next draw call
    virtual void setDepthStencil(DepthStencilView) = 0;

    /// Binds the buffer \p buffer to index \p index for the vertex pipeline
    /// \Param offset Specifies an offset into \p buffer
    virtual void setVertexBuffer(BufferView buffer, size_t index,
                                 size_t offset = 0) = 0;

    /// Specifies an offset for an already bound vertex buffer
    virtual void setVertexBufferOffset(size_t index, size_t offset) = 0;

    /// Binds the texture \p texture to the index \p index for the vertex
    /// pipeline
    virtual void setVertexTexture(TextureView texture, size_t index) = 0;

    /// Binds the texture sampler \p sampler to the index \p index for the
    /// vertex pipeline
    virtual void setVertexSampler(SamplerView sampler, size_t index) = 0;

    /// Binds the buffer \p buffer to index \p index for the fragment pipeline
    virtual void setFragmentBuffer(BufferView buffer, size_t index,
                                   size_t offset = 0) = 0;

    /// Specifies an offset for an already bound fragment buffer
    virtual void setFragmentBufferOffset(size_t index, size_t offset) = 0;

    /// Binds the texture \p texture to the index \p index for the fragment
    /// pipeline
    virtual void setFragmentTexture(TextureView, size_t index) = 0;

    /// Binds the texture sampler \p sampler to the index \p index for the
    /// vertex pipeline
    virtual void setFragmentSampler(SamplerView, size_t index) = 0;

    /// Sets the triangle fill mode for the next draw call
    virtual void setTriangleFillMode(TriangleFillMode mode) = 0;

    /// Sets the triangle cull mode for the next draw call
    virtual void setTriangleCullMode(TriangleCullMode mode) = 0;

    /// Encodes an indexed draw call
    virtual void draw(DrawDescription const& desc) = 0;

    /// Encodes a draw call without index buffer
    virtual void draw(PrimitiveType type, size_t vertexStart,
                      size_t vertexCount) = 0;

protected:
    RenderEncoder() = default;
};

/// Encodes compute commands to a command buffer
class ComputeEncoder: public CommandEncoder {
public:
    virtual void setPipeline(ComputePipelineView) = 0;

    virtual void setBuffer(BufferView, size_t index, size_t offset = 0) = 0;
    virtual void setBufferOffset(size_t index, size_t offset) = 0;
    virtual void setSampler(SamplerView, size_t index) = 0;
    virtual void setSampler(SamplerView, float lodMinClamp, float lodMaxClamp,
                            size_t index) = 0;
    virtual void setTexture(TextureView, size_t index) = 0;

    void dispatchThreads(vml::usize2 threadsPerGrid,
                         vml::usize2 threadsPerThreadgroup) {
        dispatchThreads({ threadsPerGrid, 1 }, { threadsPerThreadgroup, 1 });
    }
    virtual void dispatchThreads(vml::usize3 threadsPerGrid,
                                 vml::usize3 threadsPerThreadgroup) = 0;

protected:
    ComputeEncoder() = default;
};

/// Encodes blit commands to a command buffer
class BlitEncoder: public CommandEncoder {
public:
    ///
    virtual void copyTexture(TextureView source, TextureView dest) = 0;

    ///
    virtual void synchronizeTexture(TextureView texture, size_t slice,
                                    size_t level) = 0;

protected:
    BlitEncoder() = default;
};

} // namespace bloom

#endif // BLOOM_GPU_COMMANDBUFFER_H

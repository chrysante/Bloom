#ifndef BLOOM_GPU_RENDERCONTEXT_H
#define BLOOM_GPU_RENDERCONTEXT_H

#include <utl/vector.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

class Backbuffer;

/// Render target descriptor
struct RenderPassColorAttachmentDescription {
    /// The texture to use as a render target
    TextureHandle texture;

    ///
    mtl::float4 clearColor = 0;

    ///
    LoadAction loadAction = LoadAction::DontCare;

    ///
    StoreAction storeAction = StoreAction::Store;
};

/// Depth render target descriptor
struct RenderPassDepthAttachmentDescription {
    /// The texture to use as a depth buffer
    TextureHandle texture;

    /// The value with which the detph buffer will be cleared before drawing
    float clearDepth = 1;

    ///
    LoadAction loadAction = LoadAction::Clear;

    ///
    StoreAction storeAction = StoreAction::Store;
};

/// Argument struct for starting a render pass
struct RenderPassDescription {
    /// The color attachments to use in this pass
    utl::small_vector<RenderPassColorAttachmentDescription, 4> colorAttachments;

    /// The depth attachment to use in this pass
    RenderPassDepthAttachmentDescription depthAttachment;

    /// The number of textures in the render target
    std::size_t renderTargetArrayLength = 1;

    /// The dimensions of the render target in pixels
    mtl::usize2 renderTargetSize = 0;
};

/// Argument struct for an indexed draw call
struct DrawDescription {
    /// The index buffer to use
    BufferView indexBuffer;

    /// The index type used by `indexBuffer`
    IndexType indexType = IndexType::U32;

    /// The number of indices to draw
    std::size_t indexCount = 0;

    /// Byte offset into `indexBuffer`
    std::size_t indexBufferOffset = 0;

    /// The number of instances to draw
    std::size_t instanceCount = 1;
};

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

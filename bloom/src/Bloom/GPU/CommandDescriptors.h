#ifndef BLOOM_GPU_COMMANDDESCRIPTORS_H
#define BLOOM_GPU_COMMANDDESCRIPTORS_H

#include <vml/vml.hpp>

#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

/// Render target descriptor
struct RenderPassColorAttachmentDescription {
    /// The texture to use as a render target
    TextureHandle texture;

    ///
    vml::float4 clearColor = 0;

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
    vml::usize2 renderTargetSize = 0;
};

/// Argument struct for an indexed draw call
struct DrawDescription {
    /// The type of primitive to draw
    PrimitiveType primitiveType = PrimitiveType::Triangle;

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

} // namespace bloom

#endif // BLOOM_GPU_COMMANDDESCRIPTORS_H

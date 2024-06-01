#ifndef BLOOM_GPU_HARDWAREPRIMITIVES_H
#define BLOOM_GPU_HARDWAREPRIMITIVES_H

#include <limits>
#include <optional>

#include <utl/common.hpp>
#include <utl/vector.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/GPU/Fwd.h"
#include "Bloom/GPU/HardwareResourceHandle.h"

namespace bloom {

struct BufferDescription {
    void const* data = nullptr;
    size_t size = 0;
    StorageMode storageMode = StorageMode::GPUOnly;
};

class BufferHandle: public HardwareResourceHandle {
    friend class BufferView;

public:
    BufferHandle() = default;
    BufferHandle(void* native, Deleter deleter, BufferDescription desc):
        HardwareResourceHandle(native, deleter), desc(desc) {}

    size_t size() const { return desc.size; }
    StorageMode storageMode() const { return desc.storageMode; }

private:
    BufferDescription desc;
};

class BLOOM_API BufferView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    BufferView(BufferHandle const& buffer):
        HardwareResourceView(buffer), desc(buffer.desc) {}

    size_t size() const { return desc.size; }
    StorageMode storageMode() const { return desc.storageMode; }

public:
    BufferDescription desc;
};

struct TextureDescription {
    TextureType type = TextureType::Texture2D;
    PixelFormat pixelFormat = PixelFormat::RGBA8Unorm;
    vml::usize3 size = 0;
    size_t mipmapLevelCount = 1;
    size_t sampleCount = 1;
    size_t arrayLength = 1;
    StorageMode storageMode = StorageMode::GPUOnly;
    TextureUsage usage = TextureUsage::ShaderRead;
};

class TextureHandle: public HardwareResourceHandle {
public:
    TextureHandle() = default;
    TextureHandle(std::nullptr_t): TextureHandle() {}
    TextureHandle(void* native, Deleter deleter, TextureDescription desc):
        HardwareResourceHandle(native, deleter), desc(desc) {}

    TextureDescription description() const { return desc; }

    size_t width() const { return desc.size.x; }
    size_t height() const { return desc.size.y; }
    size_t depth() const { return desc.size.z; }
    vml::usize3 size() const { return desc.size; }

private:
    TextureDescription desc;
};

class TextureView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    TextureView(void* native, TextureDescription desc):
        HardwareResourceView(native), desc(desc) {}
    TextureView(TextureHandle const& texture):
        HardwareResourceView(texture), desc(texture.description()) {}

    TextureDescription description() const { return desc; }

    size_t width() const { return desc.size.x; }
    size_t height() const { return desc.size.y; }
    size_t depth() const { return desc.size.z; }
    vml::usize3 size() const { return desc.size; }

private:
    TextureDescription desc;
};

/// Copies the region of the texture \p source specified by \p region into \p
/// dest
void copyTextureToMemory(RenderAPI api, TextureView source, void* dest,
                         size_t bytesPerRow, vml::AABB<uint32_t, 3> region,
                         size_t mipLevel);

///
void copyTextureToMemory(RenderAPI api, TextureView source, void* dest,
                         size_t bytesPerRow, size_t bytesPerSlice,
                         vml::AABB<uint32_t, 3> region, size_t mipLevel,
                         size_t slice);

///
void copyMemoryToTexture(RenderAPI api, TextureView dest, void* data,
                         size_t bytesPerRow, vml::AABB<uint32_t, 3> region,
                         size_t mipLevel);

void copyMemoryToTexture(RenderAPI api, TextureView dest, void* data,
                         size_t bytesPerRow, size_t bytesPerImage,
                         vml::AABB<uint32_t, 3> region, size_t mipLevel,
                         size_t slice);

///
TextureHandle createSharedTextureView(RenderAPI api, TextureView texture,
                                      TextureType newType,
                                      PixelFormat newFormat,
                                      size_t firstMipLevel, size_t numMipLevels,
                                      size_t firstSlice, size_t numSlices);

class ShaderFunctionHandle: public HardwareResourceHandle {
public:
    ShaderFunctionHandle() = default;
    ShaderFunctionHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class ShaderFunctionView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    ShaderFunctionView(ShaderFunctionHandle const& function):
        HardwareResourceView(function) {}
};

struct ColorAttachmentDescription {
    PixelFormat pixelFormat = PixelFormat::Invalid;

    bool blendingEnabled = false;

    BlendFactor sourceRGBBlendFactor = BlendFactor::One;
    BlendFactor destinationRGBBlendFactor = BlendFactor::Zero;
    BlendOperation rgbBlendOperation = BlendOperation::Add;

    BlendFactor sourceAlphaBlendFactor = BlendFactor::One;
    BlendFactor destinationAlphaBlendFactor = BlendFactor::Zero;
    BlendOperation alphaBlendOperation = BlendOperation::Add;
};

struct RenderPipelineDescription {
    utl::small_vector<ColorAttachmentDescription, 4> colorAttachments;
    PixelFormat depthAttachmentPixelFormat = PixelFormat::Invalid;
    PixelFormat stencilAttachmentPixelFormat = PixelFormat::Invalid;

    ShaderFunctionHandle vertexFunction;
    ShaderFunctionHandle fragmentFunction;

    size_t rasterSampleCount = 1;
    PrimitiveTopologyClass inputPrimitiveTopology =
        PrimitiveTopologyClass::Unspecified;
};

class RenderPipelineHandle: public HardwareResourceHandle {
public:
    RenderPipelineHandle() = default;
    RenderPipelineHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class RenderPipelineView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    RenderPipelineView(RenderPipelineHandle const& pipeline):
        HardwareResourceView(pipeline) {}
};

struct ComputePipelineDescription {
    ShaderFunctionHandle computeFunction;
    bool threadGroupSizeIsMultipleOfThreadExecutionWidth = false;
    std::optional<size_t> maxTotalThreadsPerThreadgroup;
};

struct ComputePipelineParameters {
    /// The maximum number of threads in a threadgroup that you can dispatch to
    /// the pipeline
    size_t maxTotalThreadsPerThreadgroup = 0;

    /// The number of threads that the GPU executes simultaneously
    size_t threadExecutionWidth = 0;

    /// The length, in bytes, of statically allocated threadgroup memory
    size_t staticThreadgroupMemoryLength = 0;
};

class ComputePipelineHandle:
    public HardwareResourceHandle,
    public ComputePipelineParameters {
public:
    ComputePipelineHandle() = default;
    ComputePipelineHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class ComputePipelineView:
    public HardwareResourceView,
    public ComputePipelineParameters {
public:
    using HardwareResourceView::HardwareResourceView;
    ComputePipelineView(ComputePipelineHandle const& pipeline):
        HardwareResourceView(pipeline), ComputePipelineParameters(pipeline) {}
};

struct StencilDescription {
    CompareFunction stencilCompareFunction = CompareFunction::Always;

    /// Stencil is tested first. `stencilFailureOperation` declares how the
    /// stencil buffer is updated when the stencil test fails
    StencilOperation stencilFailureOperation = StencilOperation::Keep;

    /// If stencil passes, depth is tested next.  Declares what happens when the
    /// depth test fails
    StencilOperation depthFailureOperation = StencilOperation::Keep;

    /// If both the stencil and depth tests pass, declare how the stencil buffer
    /// is updated
    StencilOperation depthStencilPassOperation = StencilOperation::Keep;

    std::uint32_t readMask = 0xFFffFFff;
    std::uint32_t writeMask = 0xFFffFFff;
};

struct DepthStencilDescription {
    CompareFunction depthCompareFunction = CompareFunction::Always;
    bool depthWrite = false;
    std::optional<StencilDescription> frontFaceStencil;
    std::optional<StencilDescription> backFaceStencil;
};

class DepthStencilHandle: public HardwareResourceHandle {
public:
    DepthStencilHandle() = default;
    DepthStencilHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class DepthStencilView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    DepthStencilView(DepthStencilHandle const& depthStencil):
        HardwareResourceView(depthStencil) {}
};

struct SamplerDescription {
    SamplerMinMagFilter minFilter = SamplerMinMagFilter::Nearest;
    SamplerMinMagFilter magFilter = SamplerMinMagFilter::Nearest;
    SamplerMipFilter mipFilter = SamplerMipFilter::NotMipmapped;
    size_t maxAnisotropy = 1;
    SamplerAddressMode sAddressMode = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode tAddressMode = SamplerAddressMode::ClampToEdge;
    SamplerAddressMode rAddressMode = SamplerAddressMode::ClampToEdge;
    SamplerBorderColor borderColor = SamplerBorderColor::Clear;
    bool normalizedCoordinates = true;
    float lodMinClamp = 0;
    float lodMaxClamp = std::numeric_limits<float>::max();
    bool lodAverage = false;
    CompareFunction compareFunction = CompareFunction::Never;
    bool supportArgumentBuffers = false;
};

class SamplerHandle: public HardwareResourceHandle {
public:
    SamplerHandle() = default;
    SamplerHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class SamplerView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    SamplerView(SamplerHandle const& depthStencil):
        HardwareResourceView(depthStencil) {}
};

} // namespace bloom

#endif // BLOOM_GPU_HARDWAREPRIMITIVES_H

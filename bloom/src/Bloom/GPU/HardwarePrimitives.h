#ifndef BLOOM_GPU_HARDWAREPRIMITIVES_H
#define BLOOM_GPU_HARDWAREPRIMITIVES_H

#include <limits>
#include <optional>

#include <mtl/mtl.hpp>
#include <utl/common.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/GPU/HardwareResourceHandle.h"

namespace bloom {

enum class IndexType { uint16, uint32 };

enum class LoadAction { dontCare = 0, load = 1, clear = 2 };

enum class StoreAction {
    dontCare = 0,
    store = 1,
    multisampleResolve = 2,
};

enum class StorageMode {
    shared = 0,
    managed = 1,
    GPUOnly = 2,
};

enum class CompareFunction {
    never = 0,
    less = 1,
    equal = 2,
    lessEqual = 3,
    greater = 4,
    notEqual = 5,
    greaterEqual = 6,
    always = 7,
};

enum class TriangleFillMode { fill = 0, lines = 1 };

enum class TriangleCullMode { none = 0, front = 1, back = 2 };

enum class SamplerMinMagFilter { nearest = 0, linear = 1 };

enum class SamplerMipFilter { notMipmapped = 0, nearest = 1, linear = 2 };

enum class SamplerAddressMode {
    clampToEdge = 0,
    mirrorClampToEdge = 1,
    repeat = 2,
    mirrorRepeat = 3,
    clampToZero = 4,
    clampToBorderColor = 5,
};

enum class SamplerBorderColor {
    clear = 0,       // { 0, 0, 0, 0 }
    opaqueBlack = 1, // { 0, 0, 0, 1 }
    opaqueWhite = 2, // { 1, 1, 1, 1 }
};

enum class TextureUsage {
    none = 0,
    shaderRead = 1 << 0,
    shaderWrite = 1 << 1,
    renderTarget = 1 << 2,
    pixelFormatView = 1 << 4,
};
UTL_ENUM_OPERATORS(TextureUsage);

enum class TextureType {
    texture1D = 0,
    texture1DArray = 1,
    texture2D = 2,
    texture2DArray = 3,
    texture2DMultisample = 4,
    cubeTexture = 5,
    cubeTextureArray = 6,
    texture3D = 7,
    texture2DMultisampleArray = 8,
    textureBuffer = 9
};

enum class PixelFormat {
    invalid = 0,
    A8Unorm = 1,
    R8Unorm = 10,
    R8Unorm_sRGB = 11,
    R8Snorm = 12,
    R8Uint = 13,
    R8Sint = 14,
    R16Unorm = 20,
    R16Snorm = 22,
    R16Uint = 23,
    R16Sint = 24,
    R16Float = 25,
    RG8Unorm = 30,
    RG8Unorm_sRGB = 31,
    RG8Snorm = 32,
    RG8Uint = 33,
    RG8Sint = 34,
    B5G6R5Unorm = 40,
    A1BGR5Unorm = 41,
    ABGR4Unorm = 42,
    BGR5A1Unorm = 43,
    R32Uint = 53,
    R32Sint = 54,
    R32Float = 55,
    RG16Unorm = 60,
    RG16Snorm = 62,
    RG16Uint = 63,
    RG16Sint = 64,
    RG16Float = 65,
    RGBA8Unorm = 70,
    RGBA8Unorm_sRGB = 71,
    RGBA8Snorm = 72,
    RGBA8Uint = 73,
    RGBA8Sint = 74,
    BGRA8Unorm = 80,
    BGRA8Unorm_sRGB = 81,
    RGB10A2Unorm = 90,
    RGB10A2Uint = 91,
    RG11B10Float = 92,
    RGB9E5Float = 93,
    BGR10A2Unorm = 94,
    RG32Uint = 103,
    RG32Sint = 104,
    RG32Float = 105,
    RGBA16Unorm = 110,
    RGBA16Snorm = 112,
    RGBA16Uint = 113,
    RGBA16Sint = 114,
    RGBA16Float = 115,
    RGBA32Uint = 123,
    RGBA32Sint = 124,
    RGBA32Float = 125,
    BC1_RGBA = 130,
    BC1_RGBA_sRGB = 131,
    BC2_RGBA = 132,
    BC2_RGBA_sRGB = 133,
    BC3_RGBA = 134,
    BC3_RGBA_sRGB = 135,
    BC4_RUnorm = 140,
    BC4_RSnorm = 141,
    BC5_RGUnorm = 142,
    BC5_RGSnorm = 143,
    BC6H_RGBFloat = 150,
    BC6H_RGBUfloat = 151,
    BC7_RGBAUnorm = 152,
    BC7_RGBAUnorm_sRGB = 153,
    PVRTC_RGB_2BPP = 160,
    PVRTC_RGB_2BPP_sRGB = 161,
    PVRTC_RGB_4BPP = 162,
    PVRTC_RGB_4BPP_sRGB = 163,
    PVRTC_RGBA_2BPP = 164,
    PVRTC_RGBA_2BPP_sRGB = 165,
    PVRTC_RGBA_4BPP = 166,
    PVRTC_RGBA_4BPP_sRGB = 167,
    EAC_R11Unorm = 170,
    EAC_R11Snorm = 172,
    EAC_RG11Unorm = 174,
    EAC_RG11Snorm = 176,
    EAC_RGBA8 = 178,
    EAC_RGBA8_sRGB = 179,
    ETC2_RGB8 = 180,
    ETC2_RGB8_sRGB = 181,
    ETC2_RGB8A1 = 182,
    ETC2_RGB8A1_sRGB = 183,
    ASTC_4x4_sRGB = 186,
    ASTC_5x4_sRGB = 187,
    ASTC_5x5_sRGB = 188,
    ASTC_6x5_sRGB = 189,
    ASTC_6x6_sRGB = 190,
    ASTC_8x5_sRGB = 192,
    ASTC_8x6_sRGB = 193,
    ASTC_8x8_sRGB = 194,
    ASTC_10x5_sRGB = 195,
    ASTC_10x6_sRGB = 196,
    ASTC_10x8_sRGB = 197,
    ASTC_10x10_sRGB = 198,
    ASTC_12x10_sRGB = 199,
    ASTC_12x12_sRGB = 200,
    ASTC_4x4_LDR = 204,
    ASTC_5x4_LDR = 205,
    ASTC_5x5_LDR = 206,
    ASTC_6x5_LDR = 207,
    ASTC_6x6_LDR = 208,
    ASTC_8x5_LDR = 210,
    ASTC_8x6_LDR = 211,
    ASTC_8x8_LDR = 212,
    ASTC_10x5_LDR = 213,
    ASTC_10x6_LDR = 214,
    ASTC_10x8_LDR = 215,
    ASTC_10x10_LDR = 216,
    ASTC_12x10_LDR = 217,
    ASTC_12x12_LDR = 218,
    ASTC_4x4_HDR = 222,
    ASTC_5x4_HDR = 223,
    ASTC_5x5_HDR = 224,
    ASTC_6x5_HDR = 225,
    ASTC_6x6_HDR = 226,
    ASTC_8x5_HDR = 228,
    ASTC_8x6_HDR = 229,
    ASTC_8x8_HDR = 230,
    ASTC_10x5_HDR = 231,
    ASTC_10x6_HDR = 232,
    ASTC_10x8_HDR = 233,
    ASTC_10x10_HDR = 234,
    ASTC_12x10_HDR = 235,
    ASTC_12x12_HDR = 236,
    GBGR422 = 240,
    BGRG422 = 241,
    Depth16Unorm = 250,
    Depth32Float = 252,
    Stencil8 = 253,
    Depth24Unorm_Stencil8 = 255,
    Depth32Float_Stencil8 = 260,
    X32_Stencil8 = 261,
    X24_Stencil8 = 262,
    BGRA10_XR = 552,
    BGRA10_XR_sRGB = 553,
    BGR10_XR = 554,
    BGR10_XR_sRGB = 555
};

enum class BlendFactor {
    zero = 0,
    one = 1,
    sourceColor = 2,
    oneMinusSourceColor = 3,
    sourceAlpha = 4,
    oneMinusSourceAlpha = 5,
    destinationColor = 6,
    oneMinusDestinationColor = 7,
    destinationAlpha = 8,
    oneMinusDestinationAlpha = 9,
    sourceAlphaSaturated = 10,
    blendColor = 11,
    oneMinusBlendColor = 12,
    blendAlpha = 13,
    oneMinusBlendAlpha = 14
};

enum class BlendOperation {
    add = 0,
    subtract = 1,
    reverseSubtract = 2,
    min = 3,
    max = 4
};

enum class ColorWriteMask {
    nne = 0,
    red = 0x1 << 3,
    green = 0x1 << 2,
    blue = 0x1 << 1,
    alpha = 0x1 << 0,
    all = red | green | blue | alpha
};

enum class StencilOperation {
    keep = 0,
    zero = 1,
    replace = 2,
    incrementClamp = 3,
    decrementClamp = 4,
    invert = 5,
    incrementWrap = 6,
    decrementWrap = 7,
};

enum class PrimitiveTopologyClass {
    unspecified = 0,
    point = 1,
    line = 2,
    triangle = 3,
};

/// MARK: - Buffer
struct BLOOM_API BufferDescription {
    void const* data = nullptr;
    std::size_t size = 0;
    StorageMode storageMode = StorageMode::GPUOnly;
};

class BLOOM_API BufferHandle: public HardwareResourceHandle {
    friend class BufferView;

public:
    BufferHandle() = default;
    BufferHandle(void* native, Deleter deleter, BufferDescription desc):
        HardwareResourceHandle(native, deleter), desc(desc) {}

    std::size_t size() const { return desc.size; }
    StorageMode storageMode() const { return desc.storageMode; }

private:
    BufferDescription desc;
};

class BLOOM_API BufferView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    BufferView(BufferHandle const& buffer):
        HardwareResourceView(buffer), desc(buffer.desc) {}

    std::size_t size() const { return desc.size; }
    StorageMode storageMode() const { return desc.storageMode; }

public:
    BufferDescription desc;
};

/// MARK: - Texture
struct BLOOM_API TextureDescription {
    TextureType type = TextureType::texture2D;
    PixelFormat pixelFormat = PixelFormat::RGBA8Unorm;
    mtl::usize3 size = 1;
    std::size_t mipmapLevelCount = 1;
    std::size_t sampleCount = 1;
    std::size_t arrayLength = 1;
    StorageMode storageMode = StorageMode::GPUOnly;
    TextureUsage usage = TextureUsage::shaderRead;
};

class BLOOM_API TextureHandle: public HardwareResourceHandle {
public:
    TextureHandle() = default;
    TextureHandle(std::nullptr_t): TextureHandle() {}
    TextureHandle(void* native, Deleter deleter, TextureDescription desc):
        HardwareResourceHandle(native, deleter), desc(desc) {}

    TextureDescription description() const { return desc; }

    std::size_t width() const { return desc.size.x; }
    std::size_t height() const { return desc.size.y; }
    std::size_t depth() const { return desc.size.z; }
    mtl::usize3 size() const { return desc.size; }

private:
    TextureDescription desc;
};

class BLOOM_API TextureView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    TextureView(void* native, TextureDescription desc):
        HardwareResourceView(native), desc(desc) {}
    TextureView(TextureHandle const& texture):
        HardwareResourceView(texture), desc(texture.description()) {}

    TextureDescription description() const { return desc; }

    std::size_t width() const { return desc.size.x; }
    std::size_t height() const { return desc.size.y; }
    std::size_t depth() const { return desc.size.z; }
    mtl::usize3 size() const { return desc.size; }

private:
    TextureDescription desc;
};

/// MARK: - ShaderFunction
class BLOOM_API ShaderFunctionHandle: public HardwareResourceHandle {
public:
    ShaderFunctionHandle() = default;
    ShaderFunctionHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class BLOOM_API ShaderFunctionView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    ShaderFunctionView(ShaderFunctionHandle const& function):
        HardwareResourceView(function) {}
};

/// MARK: - RenderPipeline
struct ColorAttachmentDescription {
    PixelFormat pixelFormat = PixelFormat::invalid;

    bool blendingEnabled = false;

    BlendFactor sourceRGBBlendFactor = BlendFactor::one;
    BlendFactor destinationRGBBlendFactor = BlendFactor::zero;
    BlendOperation rgbBlendOperation = BlendOperation::add;

    BlendFactor sourceAlphaBlendFactor = BlendFactor::one;
    BlendFactor destinationAlphaBlendFactor = BlendFactor::zero;
    BlendOperation alphaBlendOperation = BlendOperation::add;
};

struct RenderPipelineDescription {
    utl::small_vector<ColorAttachmentDescription, 4> colorAttachments;
    PixelFormat depthAttachmentPixelFormat = PixelFormat::invalid;
    PixelFormat stencilAttachmentPixelFormat = PixelFormat::invalid;

    ShaderFunctionHandle vertexFunction;
    ShaderFunctionHandle fragmentFunction;

    std::size_t rasterSampleCount = 1;
    PrimitiveTopologyClass inputPrimitiveTopology =
        PrimitiveTopologyClass::unspecified;
};

class BLOOM_API RenderPipelineHandle: public HardwareResourceHandle {
public:
    RenderPipelineHandle() = default;
    RenderPipelineHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class BLOOM_API RenderPipelineView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    RenderPipelineView(RenderPipelineHandle const& pipeline):
        HardwareResourceView(pipeline) {}
};

/// MARK: - ComputePipeline
struct ComputePipelineDescription {
    ShaderFunctionHandle computeFunction;
    bool threadGroupSizeIsMultipleOfThreadExecutionWidth = false;
    std::optional<std::size_t> maxTotalThreadsPerThreadgroup;
};

struct ComputePipelineParameters {
    /// The maximum number of threads in a threadgroup that you can dispatch to
    /// the pipeline.
    std::size_t maxTotalThreadsPerThreadgroup = 0;

    /// The number of threads that the GPU executes simultaneously.
    std::size_t threadExecutionWidth = 0;

    /// The length, in bytes, of statically allocated threadgroup memory
    std::size_t staticThreadgroupMemoryLength = 0;
};

class BLOOM_API ComputePipelineHandle:
    public HardwareResourceHandle,
    public ComputePipelineParameters {
public:
    ComputePipelineHandle() = default;
    ComputePipelineHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class BLOOM_API ComputePipelineView:
    public HardwareResourceView,
    public ComputePipelineParameters {
public:
    using HardwareResourceView::HardwareResourceView;
    ComputePipelineView(ComputePipelineHandle const& pipeline):
        HardwareResourceView(pipeline), ComputePipelineParameters(pipeline) {}
};

/// MARK: - DepthStencil
struct StencilDescription {
    CompareFunction stencilCompareFunction = CompareFunction::always;

    /*! Stencil is tested first.  stencilFailureOperation declares how the
     * stencil buffer is updated when the stencil test fails. */
    StencilOperation stencilFailureOperation = StencilOperation::keep;

    /*! If stencil passes, depth is tested next.  Declare what happens when the
     * depth test fails. */
    StencilOperation depthFailureOperation = StencilOperation::keep;

    /*! If both the stencil and depth tests pass, declare how the stencil buffer
     * is updated. */
    StencilOperation depthStencilPassOperation = StencilOperation::keep;

    std::uint32_t readMask = 0xFFffFFff;
    std::uint32_t writeMask = 0xFFffFFff;
};

struct DepthStencilDescription {
    CompareFunction depthCompareFunction = CompareFunction::always;
    bool depthWrite = false;

    std::optional<StencilDescription> frontFaceStencil;
    std::optional<StencilDescription> backFaceStencil;
};

class BLOOM_API DepthStencilHandle: public HardwareResourceHandle {
public:
    DepthStencilHandle() = default;
    DepthStencilHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class BLOOM_API DepthStencilView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    DepthStencilView(DepthStencilHandle const& depthStencil):
        HardwareResourceView(depthStencil) {}
};

/// MARK: - Sampler
struct SamplerDescription {
    SamplerMinMagFilter minFilter = SamplerMinMagFilter::nearest;
    SamplerMinMagFilter magFilter = SamplerMinMagFilter::nearest;
    SamplerMipFilter mipFilter = SamplerMipFilter::notMipmapped;
    std::size_t maxAnisotropy = 1;
    SamplerAddressMode sAddressMode = SamplerAddressMode::clampToEdge;
    SamplerAddressMode tAddressMode = SamplerAddressMode::clampToEdge;
    SamplerAddressMode rAddressMode = SamplerAddressMode::clampToEdge;
    SamplerBorderColor borderColor = SamplerBorderColor::clear;
    bool normalizedCoordinates = true;
    float lodMinClamp = 0;
    float lodMaxClamp = std::numeric_limits<float>::max();
    bool lodAverage = false;
    CompareFunction compareFunction = CompareFunction::never;
    bool supportArgumentBuffers = false;
};

class BLOOM_API SamplerHandle: public HardwareResourceHandle {
public:
    SamplerHandle() = default;
    SamplerHandle(void* native, Deleter deleter):
        HardwareResourceHandle(native, deleter) {}
};

class BLOOM_API SamplerView: public HardwareResourceView {
public:
    using HardwareResourceView::HardwareResourceView;
    SamplerView(SamplerHandle const& depthStencil):
        HardwareResourceView(depthStencil) {}
};

} // namespace bloom

#endif // BLOOM_GPU_HARDWAREPRIMITIVES_H

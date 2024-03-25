#include "Bloom/Platform/Metal/MetalDevice.h"

#include <utl/utility.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Platform/Metal/MetalCommandQueue.h"
#include "Bloom/Platform/Metal/MetalRenderContext.h"
#include "Bloom/Platform/Metal/MetalSwapchain.h"
#include "Bloom/Platform/Metal/ObjCBridging.h"

using namespace vml::short_types;
using namespace bloom;

std::unique_ptr<HardwareDevice> bloom::createMetalDevice() {
    return std::make_unique<MetalDevice>();
}

/// TODO: Expose this through the UI
static id<MTLDevice> selectDevice() {
    NSArray* devices = MTLCopyAllDevices();
    for (unsigned i = 0; i < devices.count; ++i) {
        id<MTLDevice> device = [devices objectAtIndex:i];
        if (!device.isLowPower) {
            return device;
        }
    }
    return nil;
}

MetalDevice::MetalDevice() {
    device = selectDevice();
    shaderLib = [device newDefaultLibrary];
}

std::unique_ptr<Swapchain> MetalDevice::createSwapchain(
    SwapchainDescription const& desc) {
    return std::make_unique<MetalSwapchain>(device, desc);
}

std::unique_ptr<CommandQueue> MetalDevice::createCommandQueue() {
    return std::make_unique<MetalCommandQueue>([device newCommandQueue]);
}

BufferHandle MetalDevice::createBuffer(BufferDescription const& desc) {
    auto const mtlStorageMode = [&] {
        switch (desc.storageMode) {
        case StorageMode::Shared:
            return MTLResourceStorageModeShared;
        case StorageMode::Managed:
            return MTLResourceStorageModeManaged;
        case StorageMode::GPUOnly:
            return MTLResourceStorageModePrivate;
        default:
            BL_UNREACHABLE();
        };
    }();
    if (desc.data != nullptr) {
        id<MTLBuffer> buffer = [device
            newBufferWithBytes:desc.data
                        length:desc.size
                       options:MTLResourceHazardTrackingModeTracked |
                               mtlStorageMode];
        return BufferHandle(bloom_retain(buffer), bloom_release, desc);
    }
    else {
        id<MTLBuffer> buffer = [device
            newBufferWithLength:desc.size
                        options:MTLResourceHazardTrackingModeTracked |
                                mtlStorageMode];
        return BufferHandle(bloom_retain(buffer), bloom_release, desc);
    }
}

TextureHandle MetalDevice::createTexture(TextureDescription const& desc) {
    MTLTextureDescriptor* mtlDesc = [[MTLTextureDescriptor alloc] init];
    mtlDesc.textureType = (MTLTextureType)desc.type;
    mtlDesc.pixelFormat = (MTLPixelFormat)desc.pixelFormat;
    mtlDesc.width = desc.size.x;
    mtlDesc.height = desc.size.y;
    mtlDesc.depth = desc.size.z;
    mtlDesc.mipmapLevelCount = desc.mipmapLevelCount;
    mtlDesc.sampleCount = desc.sampleCount;
    mtlDesc.arrayLength = desc.arrayLength;
    mtlDesc.storageMode = (MTLStorageMode)desc.storageMode;
    mtlDesc.usage = (MTLTextureUsage)desc.usage;
    id<MTLTexture> texture = [device newTextureWithDescriptor:mtlDesc];
    return TextureHandle(bloom_retain(texture), bloom_release, desc);
}

SamplerHandle MetalDevice::createSampler(SamplerDescription const& desc) {
    MTLSamplerDescriptor* mtlDesc = [[MTLSamplerDescriptor alloc] init];
    mtlDesc.minFilter = (MTLSamplerMinMagFilter)desc.minFilter;
    mtlDesc.magFilter = (MTLSamplerMinMagFilter)desc.magFilter;
    mtlDesc.mipFilter = (MTLSamplerMipFilter)desc.mipFilter;
    mtlDesc.maxAnisotropy = desc.maxAnisotropy;
    mtlDesc.sAddressMode = (MTLSamplerAddressMode)desc.sAddressMode;
    mtlDesc.tAddressMode = (MTLSamplerAddressMode)desc.tAddressMode;
    mtlDesc.rAddressMode = (MTLSamplerAddressMode)desc.rAddressMode;
    mtlDesc.borderColor = (MTLSamplerBorderColor)desc.borderColor;
    mtlDesc.normalizedCoordinates = desc.normalizedCoordinates;
    mtlDesc.lodMinClamp = desc.lodMinClamp;
    mtlDesc.lodMaxClamp = desc.lodMaxClamp;
    mtlDesc.lodAverage = desc.lodAverage;
    mtlDesc.compareFunction = (MTLCompareFunction)desc.compareFunction;
    mtlDesc.supportArgumentBuffers = desc.supportArgumentBuffers;
    id<MTLSamplerState> sampler =
        [device newSamplerStateWithDescriptor:mtlDesc];
    return SamplerHandle(bloom_retain(sampler), bloom_release);
}

TextureHandle MetalDevice::createSharedTextureView(
    TextureView texture, TextureType newType, PixelFormat newFormat,
    std::size_t firstMipLevel, std::size_t numMipLevels, std::size_t firstSlice,
    std::size_t numSlices) {
    id<MTLTexture> mtlTexture = (__bridge id<MTLTexture>)texture.nativeHandle();
    id<MTLTexture> sharedView = [mtlTexture
        newTextureViewWithPixelFormat:(MTLPixelFormat)newFormat
                          textureType:(MTLTextureType)newType
                               levels:NSRange{ firstMipLevel, numMipLevels }
                               slices:NSRange{ firstSlice, numSlices }];
    TextureDescription desc = texture.description();
    desc.type = newType;
    desc.pixelFormat = newFormat;
    desc.mipmapLevelCount = numMipLevels;
    desc.arrayLength = numSlices;
    return TextureHandle(bloom_retain(sharedView), bloom_release, desc);
}

static MTLStencilDescriptor* toMTL(StencilDescription const& desc) {
    MTLStencilDescriptor* result = [[MTLStencilDescriptor alloc] init];
    result.stencilCompareFunction =
        (MTLCompareFunction)desc.stencilCompareFunction;
    result.stencilFailureOperation =
        (MTLStencilOperation)desc.stencilFailureOperation;
    result.depthFailureOperation =
        (MTLStencilOperation)desc.depthFailureOperation;
    result.depthStencilPassOperation =
        (MTLStencilOperation)desc.depthStencilPassOperation;
    return result;
}

DepthStencilHandle MetalDevice::createDepthStencil(
    DepthStencilDescription const& desc) {
    MTLDepthStencilDescriptor* mtlDesc =
        [[MTLDepthStencilDescriptor alloc] init];
    mtlDesc.depthCompareFunction =
        (MTLCompareFunction)desc.depthCompareFunction;
    mtlDesc.depthWriteEnabled = desc.depthWrite;
    if (desc.frontFaceStencil) {
        mtlDesc.frontFaceStencil = toMTL(*desc.frontFaceStencil);
    }
    if (desc.backFaceStencil) {
        mtlDesc.backFaceStencil = toMTL(*desc.backFaceStencil);
    }
    id<MTLDepthStencilState> result =
        [device newDepthStencilStateWithDescriptor:mtlDesc];
    return DepthStencilHandle(bloom_retain(result), bloom_release);
}

ShaderFunctionHandle MetalDevice::createFunction(std::string_view name) {
    id<MTLFunction> function = [shaderLib
        newFunctionWithName:[NSString stringWithUTF8String:name.data()]];
    return ShaderFunctionHandle(bloom_retain(function), bloom_release);
}

RenderPipelineHandle MetalDevice::createRenderPipeline(
    RenderPipelineDescription const& desc) {
    MTLRenderPipelineDescriptor* mtlDesc =
        [[MTLRenderPipelineDescriptor alloc] init];
    for (size_t index = 0; auto const& caDesc: desc.colorAttachments) {
        MTLRenderPipelineColorAttachmentDescriptor* mtlCADesc =
            [[MTLRenderPipelineColorAttachmentDescriptor alloc] init];
        mtlCADesc.pixelFormat = (MTLPixelFormat)caDesc.pixelFormat;
        mtlCADesc.blendingEnabled = caDesc.blendingEnabled;
        mtlCADesc.sourceRGBBlendFactor =
            (MTLBlendFactor)caDesc.sourceRGBBlendFactor;
        mtlCADesc.destinationRGBBlendFactor =
            (MTLBlendFactor)caDesc.destinationRGBBlendFactor;
        mtlCADesc.rgbBlendOperation =
            (MTLBlendOperation)caDesc.rgbBlendOperation;
        mtlCADesc.sourceAlphaBlendFactor =
            (MTLBlendFactor)caDesc.sourceAlphaBlendFactor;
        mtlCADesc.destinationAlphaBlendFactor =
            (MTLBlendFactor)caDesc.destinationAlphaBlendFactor;
        mtlCADesc.alphaBlendOperation =
            (MTLBlendOperation)caDesc.alphaBlendOperation;
        [mtlDesc.colorAttachments setObject:mtlCADesc atIndexedSubscript:index];
        ++index;
    }
    mtlDesc.depthAttachmentPixelFormat =
        (MTLPixelFormat)desc.depthAttachmentPixelFormat;
    mtlDesc.stencilAttachmentPixelFormat =
        (MTLPixelFormat)desc.stencilAttachmentPixelFormat;
    mtlDesc.rasterSampleCount = desc.rasterSampleCount;
    mtlDesc.inputPrimitiveTopology =
        (MTLPrimitiveTopologyClass)desc.inputPrimitiveTopology;
    [mtlDesc setVertexFunction:(__bridge id<MTLFunction>)
                                   desc.vertexFunction.nativeHandle()];
    [mtlDesc setFragmentFunction:(__bridge id<MTLFunction>)
                                     desc.fragmentFunction.nativeHandle()];
    id<MTLRenderPipelineState> pipelineState =
        [device newRenderPipelineStateWithDescriptor:mtlDesc error:nil];
    if (!pipelineState) {
        Logger::Error("Failed to create Pipeline State");
        return RenderPipelineHandle();
    }
    return RenderPipelineHandle(bloom_retain(pipelineState), bloom_release);
}

ComputePipelineHandle MetalDevice::createComputePipeline(
    ComputePipelineDescription const& desc) {
    id<MTLComputePipelineState> pipelineState = [device
        newComputePipelineStateWithFunction:(__bridge id<MTLFunction>)desc
                                                .computeFunction.nativeHandle()
                                      error:nil];
    if (!pipelineState) {
        Logger::Error("Failed to create Pipeline State");
        return ComputePipelineHandle();
    }
    auto result =
        ComputePipelineHandle(bloom_retain(pipelineState), bloom_release);
    result.maxTotalThreadsPerThreadgroup =
        pipelineState.maxTotalThreadsPerThreadgroup;
    result.threadExecutionWidth = pipelineState.threadExecutionWidth;
    result.staticThreadgroupMemoryLength =
        pipelineState.staticThreadgroupMemoryLength;
    return result;
}

void MetalDevice::fillManagedBuffer(BufferView buffer, void const* data,
                                    std::size_t size, std::size_t offset) {
    id<MTLBuffer> mtlBuffer = (__bridge id<MTLBuffer>)buffer.nativeHandle();
    char* const contents = (char*)[mtlBuffer contents];
    std::memcpy(contents + offset, data, size);
    [mtlBuffer didModifyRange:NSRange{ offset, size }];
}

void MetalDevice::reloadDefaultLibrary() {
    shaderLib = [device
        newLibraryWithFile:[NSString stringWithCString:(resourceDir() /
                                                        "default.metallib")
                                                           .c_str()
                                              encoding:NSUTF8StringEncoding]
                     error:nil];
}

void* MetalDevice::nativeHandle() { return (__bridge void*)device; }

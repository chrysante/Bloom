#include "Bloom/Platform/Metal/MetalObjects.h"

#include <utl/functional.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Platform/Metal/ObjCBridging.h"

using namespace bloom;
using namespace vml::short_types;

std::unique_ptr<HardwareDevice> bloom::createMetalDevice() {
    return std::make_unique<MetalDevice>();
}

static MTLRegion toMTL(vml::AABB<uint32_t, 3> region) {
    auto pos = region.lower_bound();
    auto size = region.size();
    return { .origin = MTLOrigin{ pos.x, pos.y, pos.z },
             .size = MTLSize{ size.x, size.y, size.z } };
}

void bloom::metalCopyData(TextureView source, void* dest, size_t bytesPerRow,
                          vml::AABB<uint32_t, 3> region, size_t mipLevel) {
    id<MTLTexture> native = (__bridge id<MTLTexture>)source.nativeHandle();
    auto mtlRegion = toMTL(region);
    [native getBytes:dest
         bytesPerRow:bytesPerRow
          fromRegion:mtlRegion
         mipmapLevel:mipLevel];
}

// MARK: MetalDevice

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

Reference<Swapchain> MetalDevice::createSwapchain(
    SwapchainDescription const& desc) {
    return allocateRef<MetalSwapchain>(device, desc);
}

Reference<CommandQueue> MetalDevice::createCommandQueue() {
    return allocateRef<MetalCommandQueue>([device newCommandQueue]);
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
    NSError* error;
    id<MTLRenderPipelineState> pipelineState =
        [device newRenderPipelineStateWithDescriptor:mtlDesc error:&error];
    if (!pipelineState) {
        Logger::Error("Failed to create Pipeline State: ",
                      [error localizedDescription].UTF8String);
        return RenderPipelineHandle();
    }
    return RenderPipelineHandle(bloom_retain(pipelineState), bloom_release);
}

ComputePipelineHandle MetalDevice::createComputePipeline(
    ComputePipelineDescription const& desc) {
    NSError* error;
    id<MTLComputePipelineState> pipelineState = [device
        newComputePipelineStateWithFunction:(__bridge id<MTLFunction>)desc
                                                .computeFunction.nativeHandle()
                                      error:&error];
    if (!pipelineState) {
        Logger::Error("Failed to create Pipeline State: ",
                      [error localizedDescription].UTF8String);
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

static std::filesystem::path getShaderLibPath() {
    auto* path = std::getenv("BLOOM_SHADER_LIB");
    if (path && std::filesystem::exists(path)) {
        return path;
    }
    return resourceDir() / "default.metallib";
}

void MetalDevice::reloadDefaultLibrary() {
    std::filesystem::path path = getShaderLibPath();
    NSURL* url =
        [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    NSError* error = nil;
    id<MTLLibrary> newLib = [device newLibraryWithURL:url error:&error];
    if (newLib) {
        shaderLib = newLib;
        Logger::Info("Successfully reloaded shaders");
    }
    else {
        Logger::Error("Failed to reload shader library ", path);
        NSLog(@"Native Error: %@", error);
    }
}

void* MetalDevice::nativeHandle() { return (__bridge void*)device; }

// MARK: MetalBackbuffer

MetalBackbuffer::MetalBackbuffer(id<CAMetalDrawable> drawable) {
    this->drawable = drawable;
}

TextureHandle MetalBackbuffer::texture() {
    return TextureHandle(bloom_retain(drawable.texture), bloom_release,
                         TextureDescription{});
}

// MARK: MetalSwapchain

MetalSwapchain::MetalSwapchain(id<MTLDevice> device,
                               SwapchainDescription const& desc) {
    layer = [[CAMetalLayer alloc] init];
    layer.device = device;
    layer.pixelFormat = (MTLPixelFormat)desc.pixelFormat;
    layer.drawableSize = CGSize{ (CGFloat)desc.size.x, (CGFloat)desc.size.y };
    layer.maximumDrawableCount = desc.backBufferCount;
    layer.displaySyncEnabled = desc.displaySync;
    layer.framebufferOnly = true;
    this->desc = desc;
}

std::unique_ptr<Backbuffer> MetalSwapchain::nextBackbuffer() {
    return std::make_unique<MetalBackbuffer>(layer.nextDrawable);
}

void MetalSwapchain::resize(vml::usize2 newSize) {
    [layer setDrawableSize:CGSize{ (CGFloat)newSize.x, (CGFloat)newSize.y }];
    desc.size = newSize;
}

// MARK: MetalCommandQueue

MetalCommandQueue::MetalCommandQueue(id<MTLCommandQueue> q) { queue = q; }

std::unique_ptr<RenderContext> MetalCommandQueue::createRenderContext() {
    return std::make_unique<MetalRenderContext>([queue commandBuffer]);
}

std::unique_ptr<ComputeContext> MetalCommandQueue::createComputeContext() {
    return std::make_unique<MetalComputeContext>([queue commandBuffer]);
}

std::unique_ptr<BlitContext> MetalCommandQueue::createBlitContext() {
    return std::make_unique<MetalBlitContext>([queue commandBuffer]);
}

// MARK: MetalRenderContext

MetalRenderContext::MetalRenderContext(id<MTLCommandBuffer> cb):
    commandBuffer(cb) {}

void MetalRenderContext::begin(RenderPassDescription const& desc) {
    MTLRenderPassDescriptor* mtlDesc = [[MTLRenderPassDescriptor alloc] init];

    for (size_t index = 0; auto ca: desc.colorAttachments) {
        MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc =
            [[MTLRenderPassColorAttachmentDescriptor alloc] init];
        colorAttachmentDesc.texture =
            (__bridge id<MTLTexture>)ca.texture.nativeHandle();
        float4 const cc = ca.clearColor;
        colorAttachmentDesc.clearColor =
            MTLClearColorMake(cc.r, cc.g, cc.b, cc.a);
        colorAttachmentDesc.loadAction =
            (MTLLoadAction)utl::to_underlying(ca.loadAction);
        colorAttachmentDesc.storeAction =
            (MTLStoreAction)utl::to_underlying(ca.storeAction);

        [mtlDesc.colorAttachments setObject:colorAttachmentDesc
                         atIndexedSubscript:index];
        ++index;
    }

    {
        MTLRenderPassDepthAttachmentDescriptor* depthAttachmentDesc =
            [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
        depthAttachmentDesc.texture =
            (__bridge id<MTLTexture>)
                desc.depthAttachment.texture.nativeHandle();
        auto const& da = desc.depthAttachment;
        depthAttachmentDesc.clearDepth = da.clearDepth;
        depthAttachmentDesc.loadAction =
            (MTLLoadAction)utl::to_underlying(da.loadAction);
        depthAttachmentDesc.storeAction =
            (MTLStoreAction)utl::to_underlying(da.storeAction);

        mtlDesc.depthAttachment = depthAttachmentDesc;
    }

    mtlDesc.renderTargetArrayLength = desc.renderTargetArrayLength;
    mtlDesc.renderTargetWidth = desc.renderTargetSize.x;
    mtlDesc.renderTargetHeight = desc.renderTargetSize.y;

    commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:mtlDesc];
}

void MetalRenderContext::end() { [commandEncoder endEncoding]; }

void MetalRenderContext::setPipeline(RenderPipelineView pipeline) {
    [commandEncoder setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)
                                               pipeline.nativeHandle()];
}

void MetalRenderContext::setDepthStencil(DepthStencilView depthStencil) {
    [commandEncoder setDepthStencilState:(__bridge id<MTLDepthStencilState>)
                                             depthStencil.nativeHandle()];
}

void MetalRenderContext::setVertexBuffer(BufferView buffer, std::size_t index,
                                         std::size_t offset) {
    [commandEncoder
        setVertexBuffer:(__bridge id<MTLBuffer>)buffer.nativeHandle()
                 offset:offset
                atIndex:index];
}

void MetalRenderContext::setVertexBufferOffset(std::size_t index,
                                               std::size_t offset) {
    [commandEncoder setVertexBufferOffset:offset atIndex:index];
}

void MetalRenderContext::setVertexTexture(TextureView texture,
                                          std::size_t index) {
    [commandEncoder
        setVertexTexture:(__bridge id<MTLTexture>)texture.nativeHandle()
                 atIndex:index];
}

void MetalRenderContext::setVertexSampler(SamplerView sampler,
                                          std::size_t index) {
    [commandEncoder setVertexSamplerState:(__bridge id<MTLSamplerState>)
                                              sampler.nativeHandle()
                                  atIndex:index];
}

void MetalRenderContext::setFragmentBuffer(BufferView buffer, std::size_t index,
                                           std::size_t offset) {
    [commandEncoder
        setFragmentBuffer:(__bridge id<MTLBuffer>)buffer.nativeHandle()
                   offset:offset
                  atIndex:index];
}

void MetalRenderContext::setFragmentBufferOffset(std::size_t index,
                                                 std::size_t offset) {
    [commandEncoder setFragmentBufferOffset:offset atIndex:index];
}

void MetalRenderContext::setFragmentTexture(TextureView texture,
                                            std::size_t index) {
    [commandEncoder
        setFragmentTexture:(__bridge id<MTLTexture>)texture.nativeHandle()
                   atIndex:index];
}

void MetalRenderContext::setFragmentSampler(SamplerView sampler,
                                            std::size_t index) {
    [commandEncoder setFragmentSamplerState:(__bridge id<MTLSamplerState>)
                                                sampler.nativeHandle()
                                    atIndex:index];
}

void MetalRenderContext::setTriangleFillMode(TriangleFillMode mode) {
    [commandEncoder setTriangleFillMode:(MTLTriangleFillMode)mode];
}

void MetalRenderContext::setTriangleCullMode(TriangleCullMode mode) {
    [commandEncoder setCullMode:(MTLCullMode)mode];
}

void MetalRenderContext::draw(DrawDescription const& desc) {
    [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                               indexCount:desc.indexCount
                                indexType:(MTLIndexType)desc.indexType
                              indexBuffer:(__bridge id<MTLBuffer>)
                                              desc.indexBuffer.nativeHandle()
                        indexBufferOffset:desc.indexBufferOffset
                            instanceCount:desc.instanceCount];
}

void MetalRenderContext::draw(std::size_t vertexStart,
                              std::size_t vertexCount) {
    [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle
                       vertexStart:vertexStart
                       vertexCount:vertexCount];
}

void MetalRenderContext::present(Backbuffer& bb) {
    auto& mtlBackbuffer = utl::down_cast<MetalBackbuffer&>(bb);
    [commandBuffer presentDrawable:mtlBackbuffer.drawable];
}

void MetalRenderContext::commit() { [commandBuffer commit]; }

// MARK: MetalComputeContext

static MTLSize toMTLSize(vml::usize3 const& size) {
    return { size.x, size.y, size.z };
}

MetalComputeContext::MetalComputeContext(id<MTLCommandBuffer> commandBuffer):
    commandBuffer(commandBuffer) {}

void MetalComputeContext::begin() {
    commandEncoder = [commandBuffer computeCommandEncoder];
}

void MetalComputeContext::end() { [commandEncoder endEncoding]; }

void MetalComputeContext::setPipeline(ComputePipelineView pipeline) {
    [commandEncoder
        setComputePipelineState:(__bridge id<MTLComputePipelineState>)
                                    pipeline.nativeHandle()];
}

void MetalComputeContext::setBuffer(BufferView buffer, std::size_t index,
                                    std::size_t offset) {
    [commandEncoder setBuffer:(__bridge id<MTLBuffer>)buffer.nativeHandle()
                       offset:offset
                      atIndex:index];
}

void MetalComputeContext::setBufferOffset(std::size_t index,
                                          std::size_t offset) {
    [commandEncoder setBufferOffset:offset atIndex:index];
}

void MetalComputeContext::setSampler(SamplerView sampler, std::size_t index) {
    [commandEncoder
        setSamplerState:(__bridge id<MTLSamplerState>)sampler.nativeHandle()
                atIndex:index];
}

void MetalComputeContext::setSampler(SamplerView sampler, float lodMinClamp,
                                     float lodMaxClamp, std::size_t index) {
    [commandEncoder
        setSamplerState:(__bridge id<MTLSamplerState>)sampler.nativeHandle()
            lodMinClamp:lodMinClamp
            lodMaxClamp:lodMaxClamp
                atIndex:index];
}

void MetalComputeContext::setTexture(TextureView texture, std::size_t index) {
    [commandEncoder setTexture:(__bridge id<MTLTexture>)texture.nativeHandle()
                       atIndex:index];
}

void MetalComputeContext::dispatchThreads(vml::usize3 threadsPerGrid,
                                          vml::usize3 threadsPerThreadgroup) {
    if (threadsPerGrid.map([](auto i) { return i == 0; }).any()) {
        return;
    }
    [commandEncoder dispatchThreads:toMTLSize(threadsPerGrid)
              threadsPerThreadgroup:toMTLSize(threadsPerThreadgroup)];
}

void MetalComputeContext::commit() { [commandBuffer commit]; }

// MARK: MetalBlitContext

MetalBlitContext::MetalBlitContext(id<MTLCommandBuffer> commandBuffer):
    commandBuffer(commandBuffer) {}

void MetalBlitContext::begin() {
    MTLBlitPassDescriptor* desc = [MTLBlitPassDescriptor blitPassDescriptor];
    commandEncoder = [commandBuffer blitCommandEncoderWithDescriptor:desc];
}

void MetalBlitContext::end() { [commandEncoder endEncoding]; }

void MetalBlitContext::commit() { [commandBuffer commit]; }

void MetalBlitContext::waitUntilCompleted() {
    [commandBuffer waitUntilCompleted];
}

void MetalBlitContext::synchronizeTexture(TextureView texture, size_t slice,
                                          size_t level) {
    id<MTLTexture> native = (__bridge id<MTLTexture>)texture.nativeHandle();
    [commandEncoder synchronizeTexture:native slice:slice level:level];
}

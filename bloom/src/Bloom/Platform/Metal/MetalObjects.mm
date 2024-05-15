#include "Bloom/Platform/Metal/MetalObjects.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <utl/functional.hpp>
#include <utl/strcat.hpp>
#include <utl/utility.hpp>

#include "Bloom/Application/ResourceUtil.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/MaterialCompiler/Preprocessor.h"
#include "Bloom/Platform/Metal/ObjCBridging.h"

using namespace bloom;
using namespace vml::short_types;

std::unique_ptr<HardwareDevice> bloom::createMetalDevice() {
    return std::make_unique<MetalDevice>();
}

static MTLOrigin toMTLOrigin(usize3 pos) {
    return MTLOrigin{ pos.x, pos.y, pos.z };
}

static MTLSize toMTLSize(usize3 size) {
    return MTLSize{ size.x, size.y, size.z };
}

static MTLRegion toMTL(vml::AABB<uint32_t, 3> region) {
    auto pos = region.lower_bound();
    auto size = region.size();
    return { .origin = toMTLOrigin(pos), .size = toMTLSize(size) };
}

static id<MTLTexture> getNative(TextureView texture) {
    return (__bridge id<MTLTexture>)texture.nativeHandle();
}

static id<MTLBuffer> getNativeBuf(BufferView buffer) {
    return (__bridge id<MTLBuffer>)buffer.nativeHandle();
}

static id<MTLFunction> getNativeFunc(ShaderFunctionView function) {
    return (__bridge id<MTLFunction>)function.nativeHandle();
}

void bloom::metalCopyTextureToMemory(TextureView source, void* dest,
                                     size_t bytesPerRow,
                                     vml::AABB<uint32_t, 3> region,
                                     size_t mipLevel) {
    id<MTLTexture> native = getNative(source);
    auto mtlRegion = toMTL(region);
    [native getBytes:dest
         bytesPerRow:bytesPerRow
          fromRegion:mtlRegion
         mipmapLevel:mipLevel];
}

void bloom::metalCopyTextureToMemory(TextureView source, void* dest,
                                     size_t bytesPerRow, size_t bytesPerSlice,
                                     vml::AABB<uint32_t, 3> region,
                                     size_t mipLevel, size_t slice) {
    id<MTLTexture> native = getNative(source);
    auto mtlRegion = toMTL(region);
    [native getBytes:dest
          bytesPerRow:bytesPerRow
        bytesPerImage:bytesPerSlice
           fromRegion:mtlRegion
          mipmapLevel:mipLevel
                slice:slice];
}

void bloom::metalCopyMemoryToTexture(TextureView dest, void* data,
                                     size_t bytesPerRow,
                                     vml::AABB<uint32_t, 3> region,
                                     size_t mipLevel) {
    id<MTLTexture> native = getNative(dest);
    auto mtlRegion = toMTL(region);
    [native replaceRegion:mtlRegion
              mipmapLevel:mipLevel
                withBytes:data
              bytesPerRow:bytesPerRow];
}

void bloom::metalCopyMemoryToTexture(TextureView dest, void* data,
                                     size_t bytesPerRow, size_t bytesPerImage,
                                     vml::AABB<uint32_t, 3> region,
                                     size_t mipLevel, size_t slice) {
    [getNative(dest) replaceRegion:toMTL(region)
                       mipmapLevel:mipLevel
                             slice:slice
                         withBytes:data
                       bytesPerRow:bytesPerRow
                     bytesPerImage:bytesPerImage];
}

TextureHandle bloom::metalCreateSharedTextureView(
    TextureView texture, TextureType newType, PixelFormat newFormat,
    size_t firstMipLevel, size_t numMipLevels, size_t firstSlice,
    size_t numSlices) {
    id<MTLTexture> sharedView = [getNative(texture)
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

Reference<ShaderCompiler> MetalDevice::createShaderCompiler() {
    return allocateRef<MetalShaderCompiler>(device);
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
    [mtlDesc setVertexFunction:getNativeFunc(desc.vertexFunction)];
    [mtlDesc setFragmentFunction:getNativeFunc(desc.fragmentFunction)];
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
        newComputePipelineStateWithFunction:getNativeFunc(desc.computeFunction)
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
                                    size_t size, size_t offset) {
    id<MTLBuffer> native = getNativeBuf(buffer);
    char* const contents = (char*)[native contents];
    std::memcpy(contents + offset, data, size);
    [native didModifyRange:NSRange{ offset, size }];
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

std::unique_ptr<CommandBuffer> MetalCommandQueue::createCommandBuffer() {
    return std::make_unique<MetalCommandBuffer>([queue commandBuffer]);
}

// MARK: MetalCommandBuffer

void MetalCommandBuffer::enqueue() { [commandBuffer enqueue]; }

void MetalCommandBuffer::commit() { [commandBuffer commit]; }

void MetalCommandBuffer::waitForCompletion() {
    [commandBuffer waitUntilCompleted];
}

static MTLRenderPassDescriptor* translate(RenderPassDescription const& desc) {
    MTLRenderPassDescriptor* result = [[MTLRenderPassDescriptor alloc] init];
    for (size_t index = 0; auto ca: desc.colorAttachments) {
        MTLRenderPassColorAttachmentDescriptor* CADesc =
            [[MTLRenderPassColorAttachmentDescriptor alloc] init];
        CADesc.texture = getNative(ca.texture);
        float4 const cc = ca.clearColor;
        CADesc.clearColor = MTLClearColorMake(cc.r, cc.g, cc.b, cc.a);
        CADesc.loadAction = (MTLLoadAction)utl::to_underlying(ca.loadAction);
        CADesc.storeAction = (MTLStoreAction)utl::to_underlying(ca.storeAction);

        [result.colorAttachments setObject:CADesc atIndexedSubscript:index];
        ++index;
    }
    MTLRenderPassDepthAttachmentDescriptor* DADesc =
        [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
    DADesc.texture = getNative(desc.depthAttachment.texture);
    auto& DA = desc.depthAttachment;
    DADesc.clearDepth = DA.clearDepth;
    DADesc.loadAction = (MTLLoadAction)utl::to_underlying(DA.loadAction);
    DADesc.storeAction = (MTLStoreAction)utl::to_underlying(DA.storeAction);
    result.depthAttachment = DADesc;
    result.renderTargetArrayLength = desc.renderTargetArrayLength;
    result.renderTargetWidth = desc.renderTargetSize.x;
    result.renderTargetHeight = desc.renderTargetSize.y;
    return result;
}

std::unique_ptr<RenderEncoder> MetalCommandBuffer::makeRenderEncoder(
    RenderPassDescription const& desc) {
    return std::make_unique<MetalRenderEncoder>(
        [commandBuffer renderCommandEncoderWithDescriptor:translate(desc)]);
}

std::unique_ptr<ComputeEncoder> MetalCommandBuffer::makeComputeEncoder() {
    return std::make_unique<MetalComputeEncoder>(
        [commandBuffer computeCommandEncoder]);
}

std::unique_ptr<BlitEncoder> MetalCommandBuffer::makeBlitEncoder() {
    return std::make_unique<MetalBlitEncoder>(
        [commandBuffer blitCommandEncoder]);
}

// MARK: MetalRenderEncoder

void MetalRenderEncoder::setPipeline(RenderPipelineView pipeline) {
    [commandEncoder setRenderPipelineState:(__bridge id<MTLRenderPipelineState>)
                                               pipeline.nativeHandle()];
}

void MetalRenderEncoder::setDepthStencil(DepthStencilView depthStencil) {
    [commandEncoder setDepthStencilState:(__bridge id<MTLDepthStencilState>)
                                             depthStencil.nativeHandle()];
}

void MetalRenderEncoder::setVertexBuffer(BufferView buffer, size_t index,
                                         size_t offset) {
    [commandEncoder setVertexBuffer:getNativeBuf(buffer)
                             offset:offset
                            atIndex:index];
}

void MetalRenderEncoder::setVertexBufferOffset(size_t index, size_t offset) {
    [commandEncoder setVertexBufferOffset:offset atIndex:index];
}

void MetalRenderEncoder::setVertexTexture(TextureView texture, size_t index) {
    [commandEncoder setVertexTexture:getNative(texture) atIndex:index];
}

void MetalRenderEncoder::setVertexSampler(SamplerView sampler, size_t index) {
    [commandEncoder setVertexSamplerState:(__bridge id<MTLSamplerState>)
                                              sampler.nativeHandle()
                                  atIndex:index];
}

void MetalRenderEncoder::setFragmentBuffer(BufferView buffer, size_t index,
                                           size_t offset) {
    [commandEncoder setFragmentBuffer:getNativeBuf(buffer)
                               offset:offset
                              atIndex:index];
}

void MetalRenderEncoder::setFragmentBufferOffset(size_t index, size_t offset) {
    [commandEncoder setFragmentBufferOffset:offset atIndex:index];
}

void MetalRenderEncoder::setFragmentTexture(TextureView texture, size_t index) {
    [commandEncoder setFragmentTexture:getNative(texture) atIndex:index];
}

void MetalRenderEncoder::setFragmentSampler(SamplerView sampler, size_t index) {
    [commandEncoder setFragmentSamplerState:(__bridge id<MTLSamplerState>)
                                                sampler.nativeHandle()
                                    atIndex:index];
}

void MetalRenderEncoder::setTriangleFillMode(TriangleFillMode mode) {
    [commandEncoder setTriangleFillMode:(MTLTriangleFillMode)mode];
}

void MetalRenderEncoder::setTriangleCullMode(TriangleCullMode mode) {
    [commandEncoder setCullMode:(MTLCullMode)mode];
}

void MetalRenderEncoder::draw(DrawDescription const& desc) {
    [commandEncoder drawIndexedPrimitives:(MTLPrimitiveType)desc.primitiveType
                               indexCount:desc.indexCount
                                indexType:(MTLIndexType)desc.indexType
                              indexBuffer:getNativeBuf(desc.indexBuffer)
                        indexBufferOffset:desc.indexBufferOffset
                            instanceCount:desc.instanceCount];
}

void MetalRenderEncoder::draw(PrimitiveType type, size_t vertexStart,
                              size_t vertexCount) {
    [commandEncoder drawPrimitives:(MTLPrimitiveType)type
                       vertexStart:vertexStart
                       vertexCount:vertexCount];
}

void MetalRenderEncoder::end() { [commandEncoder endEncoding]; }

// MARK: MetalComputeEncoder

void MetalComputeEncoder::setPipeline(ComputePipelineView pipeline) {
    [commandEncoder
        setComputePipelineState:(__bridge id<MTLComputePipelineState>)
                                    pipeline.nativeHandle()];
}

void MetalComputeEncoder::setBuffer(BufferView buffer, size_t index,
                                    size_t offset) {
    [commandEncoder setBuffer:getNativeBuf(buffer) offset:offset atIndex:index];
}

void MetalComputeEncoder::setBufferOffset(size_t index, size_t offset) {
    [commandEncoder setBufferOffset:offset atIndex:index];
}

void MetalComputeEncoder::setSampler(SamplerView sampler, size_t index) {
    [commandEncoder
        setSamplerState:(__bridge id<MTLSamplerState>)sampler.nativeHandle()
                atIndex:index];
}

void MetalComputeEncoder::setSampler(SamplerView sampler, float lodMinClamp,
                                     float lodMaxClamp, size_t index) {
    [commandEncoder
        setSamplerState:(__bridge id<MTLSamplerState>)sampler.nativeHandle()
            lodMinClamp:lodMinClamp
            lodMaxClamp:lodMaxClamp
                atIndex:index];
}

void MetalComputeEncoder::setTexture(TextureView texture, size_t index) {
    [commandEncoder setTexture:getNative(texture) atIndex:index];
}

void MetalComputeEncoder::dispatchThreads(vml::usize3 threadsPerGrid,
                                          vml::usize3 threadsPerThreadgroup) {
    if (threadsPerGrid.map([](auto i) { return i == 0; }).any()) {
        return;
    }
    [commandEncoder dispatchThreads:toMTLSize(threadsPerGrid)
              threadsPerThreadgroup:toMTLSize(threadsPerThreadgroup)];
}

void MetalComputeEncoder::end() { [commandEncoder endEncoding]; }

// MARK: MetalBlitEncoder

void MetalBlitEncoder::copyTexture(TextureView source, TextureView dest) {
    [commandEncoder copyFromTexture:getNative(source)
                          toTexture:getNative(dest)];
}

void MetalBlitEncoder::synchronizeTexture(TextureView texture, size_t slice,
                                          size_t level) {
    [commandEncoder synchronizeTexture:getNative(texture)
                                 slice:slice
                                 level:level];
}

void MetalBlitEncoder::copyBufferToTexture(
    BufferView source, size_t sourceOffset, size_t sourceBytesPerRow,
    size_t sourceBytesPerImage, usize3 sourceSize, TextureView dest,
    size_t destinationSlice, size_t destinationLevel,
    usize3 destinationOrigin) {
    [commandEncoder copyFromBuffer:getNativeBuf(source)
                      sourceOffset:sourceOffset
                 sourceBytesPerRow:sourceBytesPerRow
               sourceBytesPerImage:sourceBytesPerImage
                        sourceSize:toMTLSize(sourceSize)
                         toTexture:getNative(dest)
                  destinationSlice:destinationSlice
                  destinationLevel:destinationLevel
                 destinationOrigin:toMTLOrigin(destinationOrigin)];
}

void MetalBlitEncoder::generateMipmaps(TextureView texture) {
    [commandEncoder generateMipmapsForTexture:getNative(texture)];
}

void MetalBlitEncoder::end() { [commandEncoder endEncoding]; }

MetalShaderCompiler::MetalShaderCompiler(id<MTLDevice> device):
    device(device) {}

std::vector<ShaderFunctionHandle> MetalShaderCompiler::compile(
    std::string source, std::span<std::string const> functionNames) {
    Logger::Trace("Compiling shader source: ", source);
    source = preprocessIncludes(std::move(source), std::array{ resourceDir() });
    NSError* error = nil;
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    options.libraryType = MTLLibraryTypeExecutable;
    id<MTLLibrary> lib = [&] {
        BL_PROFILE_FUNCTION("Shader compilation");
        return [device
            newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                         options:options
                           error:&error];
    }();
    if (error != nil) {
        Logger::Error("Failed to compile shader library: ",
                      error.localizedDescription.UTF8String);
        return {};
    }
    Logger::Info("Successfully compiled shader library");
    std::vector<ShaderFunctionHandle> result;
    result.reserve(functionNames.size());
    for (auto& name: functionNames) {
        id<MTLFunction> function = [lib
            newFunctionWithName:[NSString stringWithUTF8String:name.c_str()]];
        if (!function) {
            Logger::Error("Failed to create shader function: ", name);
            return {};
        }
        result.push_back(
            ShaderFunctionHandle(bloom_retain(function), bloom_release));
    }
    return result;
}

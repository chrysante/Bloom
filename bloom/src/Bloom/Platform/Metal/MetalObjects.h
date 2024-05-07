#ifndef BLOOM_PLATFORM_METAL_METALOBJECTS_H
#define BLOOM_PLATFORM_METAL_METALOBJECTS_H

#include <memory>

#include "Bloom/GPU/BlitContext.h"
#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/ComputeContext.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/RenderContext.h"

namespace bloom {

std::unique_ptr<HardwareDevice> createMetalDevice();

void metalCopyData(TextureView source, void* dest, size_t bytesPerRow,
                   vml::AABB<uint32_t, 3> region, size_t mipLevel);

} // namespace bloom

#ifdef __OBJC__

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

namespace bloom {

class BLOOM_API MetalDevice: public HardwareDevice {
public:
    MetalDevice();

    Reference<Swapchain> createSwapchain(SwapchainDescription const&) override;
    Reference<CommandQueue> createCommandQueue() override;

    BufferHandle createBuffer(BufferDescription const&) override;
    TextureHandle createTexture(TextureDescription const&) override;
    SamplerHandle createSampler(SamplerDescription const&) override;

    TextureHandle createSharedTextureView(TextureView texture, TextureType,
                                          PixelFormat,
                                          std::size_t firstMipLevel = 0,
                                          std::size_t numMipLevels = 1,
                                          std::size_t firstSlice = 0,
                                          std::size_t numSlices = 1) override;

    DepthStencilHandle createDepthStencil(
        DepthStencilDescription const&) override;

    ShaderFunctionHandle createFunction(std::string_view name) override;
    RenderPipelineHandle createRenderPipeline(
        RenderPipelineDescription const&) override;
    ComputePipelineHandle createComputePipeline(
        ComputePipelineDescription const&) override;

    void fillManagedBuffer(BufferView, void const* data, std::size_t size,
                           std::size_t offset) override;

    void reloadDefaultLibrary() override;

    void* nativeHandle() override;

    id<MTLDevice> device;
    id<MTLLibrary> shaderLib;
};

class BLOOM_API MetalBackbuffer: public Backbuffer {
public:
    MetalBackbuffer(id<CAMetalDrawable>);

    TextureHandle texture() override;

    id<CAMetalDrawable> drawable;
};

class BLOOM_API MetalSwapchain: public Swapchain {
public:
    MetalSwapchain(id<MTLDevice>, SwapchainDescription const& desc);

    std::unique_ptr<Backbuffer> nextBackbuffer() override;

    void resize(vml::usize2 newSize) override;

    CAMetalLayer* layer;
};

class BLOOM_API MetalCommandQueue: public CommandQueue {
public:
    MetalCommandQueue(id<MTLCommandQueue>);

    std::unique_ptr<RenderContext> createRenderContext() override;
    std::unique_ptr<ComputeContext> createComputeContext() override;
    std::unique_ptr<BlitContext> createBlitContext() override;

    id<MTLCommandQueue> queue;
};

class BLOOM_API MetalRenderContext: public RenderContext {
public:
    MetalRenderContext(id<MTLCommandBuffer>);
    void begin(RenderPassDescription const&) override;
    void end() override;
    void setPipeline(RenderPipelineView) override;
    void setDepthStencil(DepthStencilView) override;
    void setVertexBuffer(BufferView, std::size_t index,
                         std::size_t offset) override;
    void setVertexBufferOffset(std::size_t index, std::size_t offset) override;
    void setVertexTexture(TextureView, std::size_t index) override;
    void setVertexSampler(SamplerView, std::size_t index) override;
    void setFragmentBuffer(BufferView, std::size_t index,
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

private:
    id<MTLCommandBuffer> commandBuffer;
    id<MTLRenderCommandEncoder> commandEncoder;
};

class MetalComputeContext: public ComputeContext {
public:
    MetalComputeContext(id<MTLCommandBuffer>);

    void begin() override;
    void end() override;

    void setPipeline(ComputePipelineView) override;

    void setBuffer(BufferView, std::size_t index,
                   std::size_t offset = 0) override;
    void setBufferOffset(std::size_t index, std::size_t offset) override;
    void setSampler(SamplerView, std::size_t index) override;
    void setSampler(SamplerView, float lodMinClamp, float lodMaxClamp,
                    std::size_t index) override;
    void setTexture(TextureView, std::size_t index) override;

    void dispatchThreads(vml::usize3 threadsPerGrid,
                         vml::usize3 threadsPerThreadgroup) override;

    void commit() override;

    id<MTLCommandBuffer> commandBuffer;
    id<MTLComputeCommandEncoder> commandEncoder;
};

class MetalBlitContext: public BlitContext {
public:
    MetalBlitContext(id<MTLCommandBuffer>);
    void begin() override;
    void end() override;
    void commit() override;
    void waitUntilCompleted() override;
    void synchronizeTexture(TextureView texture, size_t slice,
                            size_t level) override;

    id<MTLCommandBuffer> commandBuffer;
    id<MTLBlitCommandEncoder> commandEncoder;
};

} // namespace bloom

#endif // __OBJC__

#endif // BLOOM_PLATFORM_METAL_METALOBJECTS_H

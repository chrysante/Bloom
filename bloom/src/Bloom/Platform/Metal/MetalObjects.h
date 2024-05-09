#ifndef BLOOM_PLATFORM_METAL_METALOBJECTS_H
#define BLOOM_PLATFORM_METAL_METALOBJECTS_H

#include <memory>

#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/ShaderCompiler.h"

namespace bloom {

std::unique_ptr<HardwareDevice> createMetalDevice();

void metalCopyTextureToMemory(TextureView source, void* dest,
                              size_t bytesPerRow, vml::AABB<uint32_t, 3> region,
                              size_t mipLevel);

void metalCopyMemoryToTexture(TextureView dest, void* data, size_t bytesPerRow,
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

    Reference<ShaderCompiler> createShaderCompiler() override;

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

    std::unique_ptr<CommandBuffer> createCommandBuffer() final;

    id<MTLCommandQueue> queue;
};

class MetalCommandBuffer: public CommandBuffer {
public:
    MetalCommandBuffer(id<MTLCommandBuffer> commandBuffer):
        commandBuffer(commandBuffer) {}

    void enqueue() final;
    void commit() final;
    void waitForCompletion() final;
    std::unique_ptr<RenderEncoder> makeRenderEncoder(
        RenderPassDescription const& desc) final;
    std::unique_ptr<ComputeEncoder> makeComputeEncoder() final;
    std::unique_ptr<BlitEncoder> makeBlitEncoder() final;

    id<MTLCommandBuffer> commandBuffer;
};

class MetalRenderEncoder: public RenderEncoder {
public:
    MetalRenderEncoder(id<MTLRenderCommandEncoder> commandEncoder):
        commandEncoder(commandEncoder) {}

    void setPipeline(RenderPipelineView pipeline) final;
    void setDepthStencil(DepthStencilView) final;
    void setVertexBuffer(BufferView buffer, size_t index,
                         size_t offset = 0) final;
    void setVertexBufferOffset(size_t index, size_t offset) final;
    void setVertexTexture(TextureView texture, size_t index) final;
    void setVertexSampler(SamplerView sampler, size_t index) final;
    void setFragmentBuffer(BufferView buffer, size_t index,
                           size_t offset = 0) final;
    void setFragmentBufferOffset(size_t index, size_t offset) final;
    void setFragmentTexture(TextureView, size_t index) final;
    void setFragmentSampler(SamplerView, size_t index) final;
    void setTriangleFillMode(TriangleFillMode mode) final;
    void setTriangleCullMode(TriangleCullMode mode) final;
    void draw(DrawDescription const& desc) final;
    void draw(PrimitiveType type, size_t vertexStart, size_t vertexCount) final;
    void end() final;

    id<MTLRenderCommandEncoder> commandEncoder;
};

class MetalComputeEncoder: public ComputeEncoder {
public:
    MetalComputeEncoder(id<MTLComputeCommandEncoder> commandEncoder):
        commandEncoder(commandEncoder) {}

    void setPipeline(ComputePipelineView) final;
    void setBuffer(BufferView, size_t index, size_t offset = 0) final;
    void setBufferOffset(size_t index, size_t offset) final;
    void setSampler(SamplerView, size_t index) final;
    void setSampler(SamplerView, float lodMinClamp, float lodMaxClamp,
                    size_t index) final;
    void setTexture(TextureView, size_t index) final;
    void dispatchThreads(vml::usize3 threadsPerGrid,
                         vml::usize3 threadsPerThreadgroup) final;
    void end() final;

    id<MTLComputeCommandEncoder> commandEncoder;
};

class MetalBlitEncoder: public BlitEncoder {
public:
    MetalBlitEncoder(id<MTLBlitCommandEncoder> commandEncoder):
        commandEncoder(commandEncoder) {}

    void copyTexture(TextureView source, TextureView dest) final;
    void synchronizeTexture(TextureView texture, size_t slice,
                            size_t level) final;
    void end() final;

    id<MTLBlitCommandEncoder> commandEncoder;
};

class MetalShaderCompiler: public ShaderCompiler {
public:
    MetalShaderCompiler(id<MTLDevice> device);

    std::vector<ShaderFunctionHandle> compile(
        std::string source,
        std::span<std::string const> functionNames) override;

    id<MTLDevice> device;
};

} // namespace bloom

#endif // __OBJC__

#endif // BLOOM_PLATFORM_METAL_METALOBJECTS_H

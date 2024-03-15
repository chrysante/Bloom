#ifndef BLOOM_GPU_HARDWAREDEVICE_H
#define BLOOM_GPU_HARDWAREDEVICE_H

#include "Bloom/Core/Base.h"

#include <memory>

#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/GPU/Swapchain.h"

namespace bloom {

enum class RenderAPI { Metal };

struct Receipt {};

class BLOOM_API HardwareDevice {
public:
    virtual ~HardwareDevice() = default;

    static std::unique_ptr<HardwareDevice> create(RenderAPI);
    virtual std::unique_ptr<Swapchain> createSwapchain(
        SwapchainDescription const&) = 0;
    virtual std::unique_ptr<CommandQueue> createCommandQueue() = 0;

    virtual BufferHandle createBuffer(BufferDescription const&) = 0;
    virtual TextureHandle createTexture(TextureDescription const&) = 0;
    virtual SamplerHandle createSampler(SamplerDescription const&) = 0;

    virtual TextureHandle createSharedTextureView(
        TextureView texture, TextureType newType, PixelFormat newFormat,
        std::size_t firstMipLevel = 0, std::size_t numMipLevels = 1,
        std::size_t firstSlice = 0, std::size_t numSlices = 1) = 0;

    virtual DepthStencilHandle createDepthStencil(
        DepthStencilDescription const&) = 0;

    virtual ShaderFunctionHandle createFunction(std::string_view name) = 0;
    virtual RenderPipelineHandle createRenderPipeline(
        RenderPipelineDescription const&) = 0;
    virtual ComputePipelineHandle createComputePipeline(
        ComputePipelineDescription const&) = 0;

    virtual void fillManagedBuffer(BufferView, void const* data,
                                   std::size_t size,
                                   std::size_t offset = 0) = 0;

    virtual void reloadDefaultLibrary() = 0;

    virtual void* nativeHandle() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_HARDWAREDEVICE_H

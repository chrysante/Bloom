#ifndef BLOOM_GPU_HARDWAREDEVICE_H
#define BLOOM_GPU_HARDWAREDEVICE_H

#include "Bloom/Core/Base.h"

#include <memory>

#include "Bloom/GPU/CommandQueue.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/RenderContext.h"
#include "Bloom/GPU/Swapchain.h"

namespace bloom {

/// List of supported render APIs
enum class RenderAPI { Metal };

/// Represents a GPU
class BLOOM_API HardwareDevice {
public:
    ///
    virtual ~HardwareDevice() = default;

    /// Creates a device with the render API \p API
    /// For now only Metal is supported
    static std::unique_ptr<HardwareDevice> create(RenderAPI API);

    /// Creates a new swapchain created with description \p desc
    virtual std::unique_ptr<Swapchain> createSwapchain(
        SwapchainDescription const& desc) = 0;

    /// Creates a new command queue
    virtual std::unique_ptr<CommandQueue> createCommandQueue() = 0;

    /// Creates a buffer with description \p desc
    virtual BufferHandle createBuffer(BufferDescription const& desc) = 0;

    /// Creates a texture with description \p desc
    virtual TextureHandle createTexture(TextureDescription const& desc) = 0;

    /// Creates a sampler with description \p desc
    virtual SamplerHandle createSampler(SamplerDescription const& desc) = 0;

    ///
    virtual TextureHandle createSharedTextureView(
        TextureView texture, TextureType newType, PixelFormat newFormat,
        std::size_t firstMipLevel = 0, std::size_t numMipLevels = 1,
        std::size_t firstSlice = 0, std::size_t numSlices = 1) = 0;

    ///
    virtual DepthStencilHandle createDepthStencil(
        DepthStencilDescription const& desc) = 0;

    /// Creates a function handle for the function \p name
    virtual ShaderFunctionHandle createFunction(std::string_view name) = 0;

    ///
    virtual RenderPipelineHandle createRenderPipeline(
        RenderPipelineDescription const& desc) = 0;

    virtual ComputePipelineHandle createComputePipeline(
        ComputePipelineDescription const& desc) = 0;

    /// Copies `[data, data + size)` into `buffer + offset`
    /// \Pre \p buffer must be created with `StorageMode::Managed`
    virtual void fillManagedBuffer(BufferView buffer, void const* data,
                                   std::size_t size,
                                   std::size_t offset = 0) = 0;

    ///
    virtual void reloadDefaultLibrary() = 0;

    /// \Returns the native API device handle
    virtual void* nativeHandle() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_HARDWAREDEVICE_H

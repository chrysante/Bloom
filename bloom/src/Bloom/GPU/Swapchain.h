#ifndef BLOOM_GPU_SWAPCHAIN_H
#define BLOOM_GPU_SWAPCHAIN_H

#include "Bloom/Core/Base.h"
#include "Bloom/GPU/HardwarePrimitives.h"

namespace bloom {

/// Constructor options for a swapchain
struct SwapchainDescription {
    /// The pixel format that the framebuffers wil use
    PixelFormat pixelFormat = PixelFormat::BGRA8Unorm;

    /// The size of the framebuffer textures in pixels
    vml::usize2 size;

    /// The number of framebuffers to use
    std::size_t backBufferCount = 3;

    ///
    bool displaySync = true;
};

/// Framebuffer that can be presented to the screen
class BLOOM_API Backbuffer {
public:
    ///
    virtual ~Backbuffer() = default;

    /// \Returns the underlying texture
    virtual TextureHandle texture() = 0;
};

/// Series of `Backbuffer`s that can be presented to the screen
class BLOOM_API Swapchain {
public:
    virtual ~Swapchain() = default;

    /// \Returns the next framebuffer in the swapchain
    virtual std::unique_ptr<Backbuffer> nextBackbuffer() = 0;

    /// Resizes the underlying framebuffers to \p newSize pixels
    virtual void resize(vml::usize2 newSize) = 0;

    /// \Returns the descriptor that was used to create the swapchain
    SwapchainDescription const& description() const { return desc; }

    /// \Returns the size in pixels of the framebuffers
    vml::usize2 size() const { return desc.size; }

protected:
    SwapchainDescription desc;
};

} // namespace bloom

#endif // BLOOM_GPU_SWAPCHAIN_H

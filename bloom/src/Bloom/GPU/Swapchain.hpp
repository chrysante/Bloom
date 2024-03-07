#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"

namespace bloom {

struct SwapchainDescription {
    PixelFormat pixelFormat = PixelFormat::BGRA8Unorm;
    mtl::usize2 size;
    std::size_t backBufferCount = 2;
    bool displaySync = true;
};

class BLOOM_API Backbuffer {
public:
    virtual ~Backbuffer() = default;
    virtual TextureHandle texture() = 0;
};

class BLOOM_API Swapchain {
public:
    virtual ~Swapchain() = default;
    virtual std::unique_ptr<Backbuffer> nextBackbuffer() = 0;
    virtual void resize(mtl::usize2 newSize) = 0;

    SwapchainDescription const& description() const { return desc; }
    mtl::usize2 size() const { return desc.size; }

protected:
    SwapchainDescription desc;
};

} // namespace bloom

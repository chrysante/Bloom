#pragma once

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

#include "Bloom/GPU/Swapchain.hpp"

namespace bloom {

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

    void resize(mtl::usize2 newSize) override;

    CAMetalLayer* layer;
};

} // namespace bloom

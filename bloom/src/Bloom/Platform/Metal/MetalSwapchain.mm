#include "Bloom/Platform/Metal/MetalSwapchain.h"

using namespace bloom;

static void MTLDeleter(void* handle) {
    CFBridgingRelease(handle);
}

MetalBackbuffer::MetalBackbuffer(id<CAMetalDrawable> drawable) {
    this->drawable = drawable;
}

TextureHandle MetalBackbuffer::texture() {
    return TextureHandle((void*)CFBridgingRetain(drawable.texture), MTLDeleter, TextureDescription{});
}

MetalSwapchain::MetalSwapchain(id<MTLDevice> device, SwapchainDescription const& desc) {
    layer = [[CAMetalLayer alloc] init];
    layer.device = device;
    
    layer.pixelFormat = (MTLPixelFormat)desc.pixelFormat;
    
    layer.drawableSize = CGSize{ static_cast<CGFloat>(desc.size.x), static_cast<CGFloat>(desc.size.y) };
    
    layer.maximumDrawableCount = desc.backBufferCount;
    
    layer.displaySyncEnabled = desc.displaySync;
    
    this->desc = desc;
}

std::unique_ptr<Backbuffer> MetalSwapchain::nextBackbuffer() {
    return std::make_unique<MetalBackbuffer>(layer.nextDrawable);
}

void MetalSwapchain::resize(mtl::usize2 newSize) {
    [layer setDrawableSize: CGSize{ (CGFloat)newSize.x, (CGFloat)newSize.y }];
    
    desc.size = newSize;
}

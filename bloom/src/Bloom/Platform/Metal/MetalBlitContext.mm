#include "Bloom/Platform/Metal/MetalBlitContext.h"

#include <Metal/Metal.h>

#include "Bloom/GPU/HardwarePrimitives.h"

using namespace bloom;

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

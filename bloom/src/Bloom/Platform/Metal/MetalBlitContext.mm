#include "Bloom/Platform/Metal/MetalBlitContext.h"

#include "Bloom/Core/Debug.h"

using namespace bloom;

MetalBlitContext::MetalBlitContext(id<MTLCommandBuffer> commandBuffer):
    commandBuffer(commandBuffer) {}

void MetalBlitContext::commit() {
    Logger::Debug("MetalBlitContext::commit does nothing");
}

#include "Bloom/Platform/Metal/MetalBlitContext.h"

using namespace bloom;
	
MetalBlitContext::MetalBlitContext(id<MTLCommandBuffer> commandBuffer):
    commandBuffer(commandBuffer)
{
    
}

void MetalBlitContext::commit() {
    
}

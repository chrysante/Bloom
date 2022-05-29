#pragma once

#include "Bloom/GPU/BlitContext.hpp"

#include <Metal/Metal.h>

namespace bloom {
	
	class MetalBlitContext: public BlitContext {
	public:
		MetalBlitContext(id<MTLCommandBuffer>);
		void commit() override;
		
		id<MTLCommandBuffer> commandBuffer;
	};

}

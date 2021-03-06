#pragma once

#include "Bloom/GPU/ComputeContext.hpp"

#include <Metal/Metal.h>

namespace bloom {
	
	class MetalComputeContext: public ComputeContext {
	public:
		MetalComputeContext(id<MTLCommandBuffer>);
		
		void begin() override;
		void end() override;
		
		void setPipeline(ComputePipelineView) override;
		
		void setBuffer(BufferView, std::size_t index, std::size_t offset = 0) override;
		void setBufferOffset(std::size_t index, std::size_t offset) override;
		void setSampler(SamplerView, std::size_t index) override;
		void setSampler(SamplerView, float lodMinClamp, float lodMaxClamp, std::size_t index) override;
		void setTexture(TextureView, std::size_t index) override;
		
		void dispatchThreads(mtl::usize3 threadsPerGrid, mtl::usize3 threadsPerThreadgroup) override;
		
		void commit() override;
		
		id<MTLCommandBuffer> commandBuffer;
		id<MTLComputeCommandEncoder> commandEncoder;
	};

}

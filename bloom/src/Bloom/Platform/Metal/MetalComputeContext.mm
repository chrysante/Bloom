#include "MetalComputeContext.h"

namespace bloom {

	static MTLSize toMTLSize(mtl::usize3 const& size) {
		return { size.x, size.y, size.z };
	}
	
	MetalComputeContext::MetalComputeContext(id<MTLCommandBuffer> commandBuffer):
		commandBuffer(commandBuffer)
	{
		
	}
	
	void MetalComputeContext::begin() {
		commandEncoder = [commandBuffer computeCommandEncoder];
	}
	
	void MetalComputeContext::end() {
		[commandEncoder endEncoding];
	}
	
	void MetalComputeContext::setPipeline(ComputePipelineView pipeline) {
		[commandEncoder setComputePipelineState: (__bridge id<MTLComputePipelineState>)pipeline.nativeHandle()];
	}
	
	void MetalComputeContext::setBuffer(BufferView buffer, std::size_t index, std::size_t offset) {
		[commandEncoder setBuffer: (__bridge id<MTLBuffer>)buffer.nativeHandle()
						   offset: offset
						  atIndex: index];
	}
	
	void MetalComputeContext::setBufferOffset(std::size_t index, std::size_t offset) {
		[commandEncoder setBufferOffset: offset
								atIndex: index];
	}
	
	void MetalComputeContext::setSampler(SamplerView sampler, std::size_t index) {
		[commandEncoder setSamplerState: (__bridge id<MTLSamplerState>)sampler.nativeHandle()
								atIndex: index];
	}
	
	void MetalComputeContext::setSampler(SamplerView sampler, float lodMinClamp, float lodMaxClamp, std::size_t index) {
		[commandEncoder setSamplerState: (__bridge id<MTLSamplerState>)sampler.nativeHandle()
							lodMinClamp: lodMinClamp
							lodMaxClamp: lodMaxClamp
								atIndex: index];
	}
	
	void MetalComputeContext::setTexture(TextureView texture, std::size_t index) {
		[commandEncoder setTexture: (__bridge id<MTLTexture>)texture.nativeHandle()
						   atIndex: index];
	}
	
	void MetalComputeContext::dispatchThreads(mtl::usize3 threadsPerGrid,
											  mtl::usize3 threadsPerThreadgroup)
	{
		[commandEncoder dispatchThreads: toMTLSize(threadsPerGrid)
				  threadsPerThreadgroup: toMTLSize(threadsPerThreadgroup)];
	}
	
	void MetalComputeContext::commit() {
		[commandBuffer commit];
	}
	
}

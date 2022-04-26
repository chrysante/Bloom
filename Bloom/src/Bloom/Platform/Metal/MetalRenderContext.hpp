#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include "ARCPointer.hpp"
#include <Metal/Metal.hpp>

namespace bloom {
	
	void BLOOM_API MTLDeleter(void*);
	
	class BLOOM_API MetalRenderContext final: public RenderContext {
	public:
		MetalRenderContext(MTL::Device*, void* viewController);
		
		BufferHandle createBuffer(void const* data,
								  std::size_t size,
								  StorageMode) override;
		
		DepthStencilHandle createDepthStencilState(CompareFunction) override;
		
		TextureHandle createTexture(mtl::usize3 size,
									PixelFormat,
									TextureUsage,
									StorageMode) override;
		TextureHandle createArrayTexture(mtl::usize2 size,
										 int elements,
										 PixelFormat,
										 TextureUsage,
										 StorageMode) override;
		TextureHandle createRenderTarget(std::size_t width,
										 std::size_t height,
										 PixelFormat,
										 StorageMode) override;
		
		
		void fillBuffer(BufferView,
						void const* data,
						std::size_t size,
						std::size_t offset) override;
		
		void setRenderTargetColor(std::size_t index,
								  TextureView) override;
		void setClearColor(std::size_t index,
						   mtl::float4) override;
		void setRenderTargetDepth(TextureView) override;
		void setClearDepth(float) override;

		void setRenderTargetArrayLength(std::size_t) override;
		void setRenderTargetSize(mtl::uint2) override;
		void setDefaultRasterSampleCount(uint) override;
		
		void beginRenderPass() override;
		void setPipelineState(RenderPipelineView) override;
		void setDepthStencilState(DepthStencilView) override;
		
		void setVertexBuffer(BufferView,
							 int index,
							 std::size_t offset = 0) override;
		void setVertexTexture(TextureView,
							  int index) override;
		void setVertexSampler(SamplerView,
							  int index) override;
		
		void setFragmentBuffer(BufferView,
							   int index,
							   std::size_t offset = 0) override;
		void setFragmentTexture(TextureView,
								int index) override;
		void setFragmentSampler(SamplerView,
								int index) override;
		
		void setTriangleFillMode(TriangleFillMode) override;
		void setTriangleCullMode(TriangleCullMode) override;
		
		void drawIndexed(BufferView indices,
						 IndexType) override;
		
		void drawIndexedInstances(BufferView indices,
								  IndexType,
								  std::size_t instances) override;
		
		RenderPassHandle commit() override;
		
		MTL::Device*       device()         { return _device.get(); }
		MTL::CommandQueue* commandQueue()   { return _commandQueue.get(); }
		void*              viewController() { return _viewController; }
		
	private:
		void resetDescriptorValues();
		
	private:
		ARCPointer<MTL::Device> _device;
		void* _viewController;
		ARCPointer<MTL::CommandQueue> _commandQueue;
		
		ARCPointer<MTL::RenderPassDescriptor> _renderPassDesc;
		MTL::CommandBuffer* _commandBuffer;
		MTL::RenderCommandEncoder* _commandEncoder;
		
		std::array<TextureView, 8> currentRenderTargetColor;
		std::array<mtl::float4, 8> clearColor{};
		TextureView currentRenderTargetDepth;
		float clearDepth = 0;
		uint renderTargetArrayLength = 0;
		mtl::uint2 renderTargetSize = 0;
		uint defaultRasterSampleCount = 0;
	};
	
}

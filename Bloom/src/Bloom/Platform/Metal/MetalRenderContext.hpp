#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Graphics/RenderContext.hpp"
#include "ARCPointer.hpp"
#include <Metal/Metal.hpp>

namespace bloom {
	
	void BLOOM_API MTLDeleter(void*);
	
	class BLOOM_API MetalRenderContext: public RenderContext {
	public:
		MetalRenderContext(MTL::Device*, void* viewController);
		
		BufferHandle createVertexBuffer(void const* data, std::size_t size) final;
		BufferHandle createIndexBuffer(std::span<std::uint16_t>) final;
		BufferHandle createIndexBuffer(std::span<std::uint32_t>) final;
		BufferHandle createUniformBuffer(void const* data, std::size_t size) final;
		
		DepthStencilHandle createDepthStencilState(CompareFunction) final;
		
		TextureHandle createRenderTarget(std::size_t width, std::size_t height, PixelFormat, StorageMode) final;
		
		void fillBuffer(BufferView, void const* data, std::size_t size, std::size_t offset) final;
		
		void setRenderTargetColor(std::size_t index, TextureView) final;
		void setClearColor(std::size_t index, mtl::float4) final;
		void setRenderTargetDepth(TextureView) final;
		void setClearDepth(float) final;
		
		void beginRenderPass() final;
		void setPipelineState(RenderPipelineView) final;
		void setDepthStencilState(DepthStencilView) final;
		
		void setVertexBuffer(BufferView, int index, std::size_t offset = 0) final;
		void setVertexTexture(TextureView, int index) final;
		void setVertexSampler(SamplerView, int index) final;
		
		void setFragmentBuffer(BufferView, int index, std::size_t offset = 0) final;
		void setFragmentTexture(TextureView, int index) final;
		void setFragmentSampler(SamplerView, int index) final;
		
		void setTriangleFillMode(TriangleFillMode) final;
		
		void drawIndexed(BufferView indices, IndexType) final;
		
		RenderPassHandle commit() final;
		
		MTL::Device* device() { return _device.get(); }
		MTL::CommandQueue* commandQueue() { return _commandQueue.get(); }
		void* viewController() { return _viewController; }
		
	private:
		BufferHandle createBuffer(void const* data, std::size_t size, unsigned mode);
		
		void eraseRenderTargets();
		
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
	};
	
}

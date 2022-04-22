#pragma once

#include <mtl/mtl.hpp>
#include <utl/memory.hpp>
#include <span>

#include "Bloom/Core/Base.hpp"
#include "RenderAPI.hpp"
#include "RenderPrimitives.hpp"
#include "RenderPassHandle.hpp"

namespace bloom {
	
	enum class IndexType {
		uint16, uint32
	};
	
	
	
	class BLOOM_API RenderContext {
	protected:
		RenderContext() = default;
		
	public:
		virtual ~RenderContext() = default;
		
		virtual BufferHandle createVertexBuffer(void const* data, std::size_t size) = 0;
		virtual BufferHandle createIndexBuffer(std::span<std::uint16_t>) = 0;
		virtual BufferHandle createIndexBuffer(std::span<std::uint32_t>) = 0;
		virtual BufferHandle createUniformBuffer(void const* data, std::size_t size) = 0;
		
		virtual DepthStencilHandle createDepthStencilState(CompareFunction) = 0;
		
		virtual TextureHandle createRenderTarget(std::size_t width, std::size_t height, PixelFormat, StorageMode) = 0;
//		virtual Texture createTexture(std::size_t width, std::size_t height, std::size_t depth) = 0;
		
		virtual void fillBuffer(BufferView, void const* data, std::size_t size, std::size_t offset = 0) = 0;
		
		virtual void setRenderTargetColor(std::size_t index, TextureView) = 0;
		virtual void setClearColor(std::size_t index, mtl::float4) = 0;
		
		virtual void setRenderTargetDepth(TextureView) = 0;
		virtual void setClearDepth(float) = 0;
		
		virtual void beginRenderPass() = 0;
		virtual void setPipelineState(RenderPipelineView) = 0;
		virtual void setDepthStencilState(DepthStencilView) = 0;
		
		virtual void setVertexBuffer(BufferView, int index, std::size_t offset = 0) = 0;
		virtual void setVertexTexture(TextureView, int index) = 0;
		virtual void setVertexSampler(SamplerView, int index) = 0;
		
		virtual void setFragmentBuffer(BufferView, int index, std::size_t offset = 0) = 0;
		virtual void setFragmentTexture(TextureView, int index) = 0;
		virtual void setFragmentSampler(SamplerView, int index) = 0;
		
		virtual void setTriangleFillMode(TriangleFillMode) = 0;
		
		virtual void drawIndexed(BufferView indices, IndexType) = 0;
		
		virtual RenderPassHandle commit() = 0;
		
	private:
		
	};
	
}

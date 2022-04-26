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
		
		virtual BufferHandle createBuffer(void const* data, std::size_t size, StorageMode) = 0;
		
		template <typename VertexType>
		BufferHandle createVertexBuffer(std::span<VertexType const> vertices) {
			return createBuffer(vertices.data(), vertices.size() * sizeof(VertexType), StorageMode::Shared);
		}
		BufferHandle createVertexBuffer(void const* data, std::size_t size) {
			return createBuffer(data, size, StorageMode::Shared);
		}
		BufferHandle createIndexBuffer(std::span<std::uint16_t const> indices) {
			return createBuffer(indices.data(), indices.size() * 2, StorageMode::Shared);
		}
		BufferHandle createIndexBuffer(std::span<std::uint32_t const> indices) {
			return createBuffer(indices.data(), indices.size() * 4, StorageMode::Shared);
		}
		BufferHandle createUniformBuffer(void const* data, std::size_t size) {
			return createBuffer(data, size, StorageMode::Managed);
		}
		
		virtual DepthStencilHandle createDepthStencilState(CompareFunction) = 0;

		virtual TextureHandle createTexture(mtl::usize3 size, PixelFormat, TextureUsage, StorageMode) = 0;
		virtual TextureHandle createArrayTexture(mtl::usize2 size, int elements, PixelFormat, TextureUsage, StorageMode) = 0;
		virtual TextureHandle createRenderTarget(std::size_t width, std::size_t height, PixelFormat, StorageMode) = 0;
		
//		virtual TextureHandle populateTexture(TextureView, void const* data) = 0;
		
		virtual void fillBuffer(BufferView, void const* data, std::size_t size, std::size_t offset = 0) = 0;
		
		virtual void setRenderTargetColor(std::size_t index, TextureView) = 0;
		virtual void setClearColor(std::size_t index, mtl::float4) = 0;
		
		virtual void setRenderTargetDepth(TextureView) = 0;
		virtual void setClearDepth(float) = 0;
		
		virtual void setRenderTargetArrayLength(std::size_t) = 0;
		virtual void setRenderTargetSize(mtl::uint2) = 0;
		virtual void setDefaultRasterSampleCount(uint) = 0;
		
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
		virtual void setTriangleCullMode(TriangleCullMode) = 0;
		
		virtual void drawIndexed(BufferView indices, IndexType) = 0;
		virtual void drawIndexedInstances(BufferView indices, IndexType, std::size_t instances) = 0;
		
		virtual RenderPassHandle commit() = 0;
		
	private:
		
	};
	
}

#pragma once

#include "Bloom/Core/Base.hpp"

#include "HardwarePrimitives.hpp"

#include <utl/vector.hpp>

namespace bloom {
	
	class Backbuffer;
	
	struct RenderPassColorAttachmentDescription {
		TextureHandle texture;
		mtl::float4 clearColor = 0;
		LoadAction loadAction = LoadAction::dontCare;
		StoreAction storeAction = StoreAction::store;
	};
	
	struct RenderPassDepthAttachmentDescription {
		TextureHandle texture;
		float clearDepth = 1;
		LoadAction loadAction = LoadAction::clear;
		StoreAction storeAction = StoreAction::store;
	};
	
	struct RenderPassDescription {
		utl::small_vector<RenderPassColorAttachmentDescription, 4> colorAttachments;
		RenderPassDepthAttachmentDescription depthAttachment;
		std::size_t renderTargetArrayLength = 1;
		mtl::usize2 renderTargetSize = 0;
	};
	
	struct DrawDescription {
		BufferView indexBuffer;
		IndexType indexType = IndexType::uint32;
		std::size_t indexCount = 0;
		std::size_t indexBufferOffset = 0;
		std::size_t instanceCount = 1;
	};
	
	class BLOOM_API RenderContext {
	public:
		virtual ~RenderContext() = default;
		
		virtual void begin(RenderPassDescription const&) = 0;
		virtual void end() = 0;
		
		virtual void setPipeline(RenderPipelineView) = 0;
		virtual void setDepthStencil(DepthStencilView) = 0;
		
		virtual void setVertexBuffer(BufferView, std::size_t index, std::size_t offset = 0) = 0;
		virtual void setVertexBufferOffset(std::size_t index, std::size_t offset) = 0;
		virtual void setVertexTexture(TextureView, std::size_t index) = 0;
		virtual void setVertexSampler(SamplerView, std::size_t index) = 0;
		
		virtual void setFragmentBuffer(BufferView, std::size_t index, std::size_t offset = 0) = 0;
		virtual void setFragmentBufferOffset(std::size_t index, std::size_t offset) = 0;
		virtual void setFragmentTexture(TextureView, std::size_t index) = 0;
		virtual void setFragmentSampler(SamplerView, std::size_t index) = 0;
		
		virtual void setTriangleFillMode(TriangleFillMode) = 0;
		virtual void setTriangleCullMode(TriangleCullMode) = 0;
		
		virtual void draw(DrawDescription const&) = 0;
		virtual void draw(std::size_t vertexStart, std::size_t vertexCount) = 0;
		
		virtual void present(Backbuffer&) = 0;
		
		virtual void commit() = 0;
	};
	
}

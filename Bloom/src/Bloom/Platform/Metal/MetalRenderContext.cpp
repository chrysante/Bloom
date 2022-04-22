#include "MetalRenderContext.hpp"

#include "Bloom/Core/Debug.hpp"

namespace bloom {
	
	void MTLDeleter(void* handle) {
		auto* object = (NS::Object*)handle;
		object->release();
	}
	
	MetalRenderContext::MetalRenderContext(MTL::Device* device, void* viewController):
		_device(device),
		_viewController(viewController),
		_commandQueue(device->newCommandQueue())
	{
		
	}
	
	BufferHandle MetalRenderContext::createVertexBuffer(void const* data, std::size_t size) {
		return createBuffer(data, size, MTL::ResourceStorageModeShared);
	}
	
	BufferHandle MetalRenderContext::createIndexBuffer(std::span<std::uint16_t> indices) {
		return createBuffer(indices.data(), indices.size() * 2, MTL::ResourceStorageModeShared);
	}
	
	BufferHandle MetalRenderContext::createIndexBuffer(std::span<std::uint32_t> indices) {
		return createBuffer(indices.data(), indices.size() * 4, MTL::ResourceStorageModeShared);
	}
	
	BufferHandle MetalRenderContext::createUniformBuffer(void const* data, std::size_t size) {
		return createBuffer(data, size, MTL::ResourceStorageModeManaged);
	}
	
	DepthStencilHandle MetalRenderContext::createDepthStencilState(CompareFunction cmp) {
		ARCPointer dsDesc = MTL::DepthStencilDescriptor::alloc()->init();
		dsDesc->setDepthWriteEnabled(true);
		dsDesc->setDepthCompareFunction((MTL::CompareFunction)cmp);
		return DepthStencilHandle(_device->newDepthStencilState(dsDesc.get()), MTLDeleter);
	}
	
	TextureHandle MetalRenderContext::createRenderTarget(std::size_t width, std::size_t height, PixelFormat format, StorageMode storageMode) {
		ARCPointer texDescriptor = MTL::TextureDescriptor::alloc()->init();
		texDescriptor->setTextureType(MTL::TextureType2D);
		texDescriptor->setWidth(width);
		texDescriptor->setHeight(height);
		texDescriptor->setPixelFormat((MTL::PixelFormat)format);
		texDescriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
		
		texDescriptor->setStorageMode((MTL::StorageMode)storageMode);
		
		return TextureHandle(device()->newTexture(texDescriptor.get()), MTLDeleter,
							 width, height, 1);
		
//		if ((1)) {
//			texDescriptor->setStorageMode(MTL::StorageModePrivate);
//			texDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
//			TextureHandle depth(device()->newTexture(texDescriptor.get()), MTLDeleter,
//								width, height, 1);
//			return RenderTarget(std::move(color), std::move(depth));
//		}
//		
//		return RenderTarget(std::move(color));
	}
	
	void MetalRenderContext::fillBuffer(BufferView view, void const* data, std::size_t size, std::size_t offset) {
		auto* buffer = (MTL::Buffer*)view.nativeHandle();
		std::memcpy((char*)buffer->contents() + offset, data, size);
		buffer->didModifyRange(NS::Range(offset, size));
	}
	
	void MetalRenderContext::setRenderTargetColor(std::size_t index, TextureView texture) {
		bloomExpect(index < 8);
		currentRenderTargetColor[index] = texture;
	}
	
	void MetalRenderContext::setClearColor(std::size_t index, mtl::float4 color) {
		bloomExpect(index < 8);
		clearColor[index] = color;
	}
	
	void MetalRenderContext::setRenderTargetDepth(TextureView texture) {
		currentRenderTargetDepth = texture;
	}
	
	void MetalRenderContext::setClearDepth(float depth) {
		clearDepth = depth;
	}
	
	
	void MetalRenderContext::beginRenderPass() {
		_renderPassDesc = MTL::RenderPassDescriptor::alloc()->init();
		for (std::size_t index = 0;
			 auto texture: currentRenderTargetColor)
		{
			if (!texture) {
				continue;
			}
			_renderPassDesc->colorAttachments()->object(index)->setTexture((MTL::Texture*)texture.nativeHandle());
			_renderPassDesc->colorAttachments()->object(index)->setLoadAction(MTL::LoadActionClear);
			_renderPassDesc->colorAttachments()->object(index)->setClearColor(MTL::ClearColor(clearColor[index].r,
																							  clearColor[index].g,
																							  clearColor[index].b,
																							  clearColor[index].a));
			_renderPassDesc->colorAttachments()->object(index)->setStoreAction(MTL::StoreActionStore);
			++index;
		}
		if (currentRenderTargetDepth) {
			_renderPassDesc->depthAttachment()->setTexture((MTL::Texture*)currentRenderTargetDepth.nativeHandle());
		}
		
		_commandBuffer = _commandQueue->commandBuffer();
		_commandEncoder = _commandBuffer->renderCommandEncoder(_renderPassDesc.get());
	}
	
	void MetalRenderContext::setPipelineState(RenderPipelineView pipeline) {
		_commandEncoder->setRenderPipelineState((MTL::RenderPipelineState*)pipeline.nativeHandle());
	}
	
	void MetalRenderContext::setDepthStencilState(DepthStencilView depthStencil) {
		_commandEncoder->setDepthStencilState((MTL::DepthStencilState*)depthStencil.nativeHandle());
	}
	
	void MetalRenderContext::setVertexBuffer(BufferView buffer, int index, std::size_t offset) {
		_commandEncoder->setVertexBuffer((MTL::Buffer*)buffer.nativeHandle(), offset, index);
	}
	
	void MetalRenderContext::setVertexTexture(TextureView texture, int index) {
		_commandEncoder->setVertexTexture((MTL::Texture*)texture.nativeHandle(), index);
	}
	
	void MetalRenderContext::setVertexSampler(SamplerView sampler, int index) {
		_commandEncoder->setVertexSamplerState((MTL::SamplerState*)sampler.nativeHandle(), index);
	}
	
	void MetalRenderContext::setFragmentBuffer(BufferView buffer, int index, std::size_t offset) {
		_commandEncoder->setFragmentBuffer((MTL::Buffer*)buffer.nativeHandle(), offset, index);
	}
	
	void MetalRenderContext::setFragmentTexture(TextureView texture, int index) {
		_commandEncoder->setFragmentTexture((MTL::Texture*)texture.nativeHandle(), index);
	}
	
	void MetalRenderContext::setFragmentSampler(SamplerView sampler, int index) {
		_commandEncoder->setFragmentSamplerState((MTL::SamplerState*)sampler.nativeHandle(), index);
	}
	
	void MetalRenderContext::setTriangleFillMode(TriangleFillMode fillMode) {
		_commandEncoder->setTriangleFillMode((MTL::TriangleFillMode)fillMode);
	}
	
	void MetalRenderContext::drawIndexed(BufferView indices, IndexType type) {
		std::size_t const indexSize = type == IndexType::uint32 ? 4 : 2;
		_commandEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
											   /* indexCount = */ indices.size() / indexSize,
											   type == IndexType::uint32 ? MTL::IndexTypeUInt32 : MTL::IndexTypeUInt16,
											   (MTL::Buffer*)indices.nativeHandle(), 0);
	}
	
	RenderPassHandle MetalRenderContext::commit() {
		_commandEncoder->endEncoding();
		_commandBuffer->commit();
		
		eraseRenderTargets();
		
		return RenderPassHandle([cmdbuf = _commandBuffer]{ cmdbuf->waitUntilCompleted(); });
	}
	
	BufferHandle MetalRenderContext::createBuffer(void const* data, std::size_t size, unsigned mode) {
		if (data) {
			return BufferHandle(_device->newBuffer(data, size, mode), MTLDeleter, size);
		}
		else {
			return BufferHandle(_device->newBuffer(size, mode), MTLDeleter, size);
		}
	}
	
	void MetalRenderContext::eraseRenderTargets() {
		for (auto& texture: currentRenderTargetColor) {
			texture = nullptr;
		}
		currentRenderTargetDepth = nullptr;
	}
}

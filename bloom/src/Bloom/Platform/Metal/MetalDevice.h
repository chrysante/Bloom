#pragma once

#include "Bloom/GPU/HardwareDevice.hpp"
#include <memory>

namespace bloom {
	
	std::unique_ptr<HardwareDevice> createMetalDevice();
	
}

#ifdef __OBJC__

#include <Metal/Metal.h>

namespace bloom {
	
	class BLOOM_API MetalDevice: public HardwareDevice {
	public:
		MetalDevice();
		
		std::unique_ptr<Swapchain> createSwapchain(SwapchainDescription const&) override;
		std::unique_ptr<CommandQueue> createCommandQueue() override;
		
		BufferHandle createBuffer(BufferDescription const&) override;
		TextureHandle createTexture(TextureDescription const&) override;
		SamplerHandle createSampler(SamplerDescription const&) override;
		
		TextureHandle createSharedTextureView(TextureView texture,
											  TextureType,
											  PixelFormat,
											  std::size_t firstMipLevel = 0, std::size_t numMipLevels = 1,
											  std::size_t firstSlice = 0, std::size_t numSlices = 1) override;
		
		DepthStencilHandle createDepthStencil(DepthStencilDescription const&) override;
		
		ShaderFunctionHandle createFunction(std::string_view name) override;
		RenderPipelineHandle createRenderPipeline(RenderPipelineDescription const&) override;
		ComputePipelineHandle createComputePipeline(ComputePipelineDescription const&) override;
		
		void fillManagedBuffer(BufferView, void const* data, std::size_t size, std::size_t offset) override;

		void reloadDefaultLibrary() override;
		
		void* nativeHandle() override;
		
		id<MTLDevice> device;
		id<MTLLibrary> shaderLib;
	};
	
}

#endif


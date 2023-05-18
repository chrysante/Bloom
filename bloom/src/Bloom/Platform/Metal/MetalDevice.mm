#include "MetalDevice.h"

#include "MetalRenderContext.h"
#include "MetalSwapchain.h"
#include "MetalCommandQueue.h"


#include "Bloom/Core/Debug.hpp"
#include "Bloom/Application/ResourceUtil.hpp"

#include <utl/utility.hpp>

using namespace mtl::short_types;

namespace bloom {
	
	std::unique_ptr<HardwareDevice> createMetalDevice() {
		return std::make_unique<MetalDevice>();
	}
	
	static void MTLDeleter(void* handle) {
		CFBridgingRelease(handle);
	}
	
	MetalDevice::MetalDevice() {
		device = MTLCreateSystemDefaultDevice();
		shaderLib = [device newDefaultLibrary];
	}
	
	std::unique_ptr<Swapchain> MetalDevice::createSwapchain(SwapchainDescription const& desc) {
		return std::make_unique<MetalSwapchain>(device, desc);
	}
	
	std::unique_ptr<CommandQueue> MetalDevice::createCommandQueue() {
		return std::make_unique<MetalCommandQueue>([device newCommandQueue]);
	}
	
	BufferHandle MetalDevice::createBuffer(BufferDescription const& desc) {
		auto const mtlStorageMode = [&]{
			switch (desc.storageMode) {
				case StorageMode::shared: return MTLResourceStorageModeShared;
				case StorageMode::managed: return MTLResourceStorageModeManaged;
				case StorageMode::GPUOnly: return MTLResourceStorageModePrivate;
				default: bloomDebugfail();
			};
		}();
		
		if (desc.data != nullptr) {
			id<MTLBuffer> buffer = [device newBufferWithBytes: desc.data
													   length: desc.size
													  options: MTLResourceHazardTrackingModeTracked | mtlStorageMode];
			
			return BufferHandle((void*)CFBridgingRetain(buffer), MTLDeleter, desc);
		}
		else {
			id<MTLBuffer> buffer = [device newBufferWithLength: desc.size
													   options: MTLResourceHazardTrackingModeTracked | mtlStorageMode];
			
			return BufferHandle((void*)CFBridgingRetain(buffer), MTLDeleter, desc);
		}
	}
	
	TextureHandle MetalDevice::createTexture(TextureDescription const& desc) {
		MTLTextureDescriptor* mtlDesc = [[MTLTextureDescriptor alloc] init];
		mtlDesc.textureType = (MTLTextureType)desc.type;
		mtlDesc.pixelFormat = (MTLPixelFormat)desc.pixelFormat;
		mtlDesc.width = desc.size.x;
		mtlDesc.height = desc.size.y;
		mtlDesc.depth = desc.size.z;
		mtlDesc.mipmapLevelCount = desc.mipmapLevelCount;
		mtlDesc.sampleCount = desc.sampleCount;
		mtlDesc.arrayLength = desc.arrayLength;
		mtlDesc.storageMode = (MTLStorageMode)desc.storageMode;
		mtlDesc.usage = (MTLTextureUsage)desc.usage;
		id<MTLTexture> texture = [device newTextureWithDescriptor: mtlDesc];
		return TextureHandle((void*)CFBridgingRetain(texture), MTLDeleter, desc);
	}
	
	SamplerHandle MetalDevice::createSampler(SamplerDescription const& desc) {
		MTLSamplerDescriptor* mtlDesc = [[MTLSamplerDescriptor alloc] init];
		
		mtlDesc.minFilter = (MTLSamplerMinMagFilter)desc.minFilter;
		mtlDesc.magFilter = (MTLSamplerMinMagFilter)desc.magFilter;
		mtlDesc.mipFilter = (MTLSamplerMipFilter)desc.mipFilter;
		mtlDesc.maxAnisotropy = desc.maxAnisotropy;
		mtlDesc.sAddressMode = (MTLSamplerAddressMode)desc.sAddressMode;
		mtlDesc.tAddressMode = (MTLSamplerAddressMode)desc.tAddressMode;
		mtlDesc.rAddressMode = (MTLSamplerAddressMode)desc.rAddressMode;
		mtlDesc.borderColor = (MTLSamplerBorderColor)desc.borderColor;
		mtlDesc.normalizedCoordinates = desc.normalizedCoordinates;
		mtlDesc.lodMinClamp = desc.lodMinClamp;
		mtlDesc.lodMaxClamp = desc.lodMaxClamp;
		mtlDesc.lodAverage = desc.lodAverage;
		mtlDesc.compareFunction = (MTLCompareFunction)desc.compareFunction;
		mtlDesc.supportArgumentBuffers = desc.supportArgumentBuffers;
		
	
		id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor: mtlDesc];
		return SamplerHandle((void*)CFBridgingRetain(sampler), MTLDeleter);
	}
	
	TextureHandle MetalDevice::createSharedTextureView(TextureView texture,
													   TextureType newType,
													   PixelFormat newFormat,
													   std::size_t firstMipLevel, std::size_t numMipLevels,
													   std::size_t firstSlice, std::size_t numSlices)
	{
		id<MTLTexture> mtlTexture = (__bridge id<MTLTexture>)texture.nativeHandle();
		id<MTLTexture> sharedView = [mtlTexture newTextureViewWithPixelFormat: (MTLPixelFormat)newFormat
																  textureType: (MTLTextureType)newType
																	   levels: NSRange{ firstMipLevel, numMipLevels }
																	   slices: NSRange{ firstSlice, numSlices }];
		
		TextureDescription desc = texture.description();
		desc.type = newType;
		desc.pixelFormat = newFormat;
		desc.mipmapLevelCount = numMipLevels;
		desc.arrayLength = numSlices;
		
		return TextureHandle((void*)CFBridgingRetain(sharedView), MTLDeleter, desc);
	}
	
	static MTLStencilDescriptor* toMTL(StencilDescription const& desc) {
		MTLStencilDescriptor* result = [[MTLStencilDescriptor alloc] init];
		result.stencilCompareFunction = (MTLCompareFunction)desc.stencilCompareFunction;
		result.stencilFailureOperation = (MTLStencilOperation)desc.stencilFailureOperation;
		result.depthFailureOperation = (MTLStencilOperation)desc.depthFailureOperation;
		result.depthStencilPassOperation = (MTLStencilOperation)desc.depthStencilPassOperation;
		return result;
	}
	
	DepthStencilHandle MetalDevice::createDepthStencil(DepthStencilDescription const& desc) {
		MTLDepthStencilDescriptor* mtlDesc = [[MTLDepthStencilDescriptor alloc] init];
		
		mtlDesc.depthCompareFunction = (MTLCompareFunction)desc.depthCompareFunction;
		mtlDesc.depthWriteEnabled = desc.depthWrite;
		
		if (desc.frontFaceStencil) {
			mtlDesc.frontFaceStencil = toMTL(*desc.frontFaceStencil);
		}
		if (desc.backFaceStencil) {
			mtlDesc.backFaceStencil = toMTL(*desc.backFaceStencil);
		}
		
		id<MTLDepthStencilState> result = [device newDepthStencilStateWithDescriptor: mtlDesc];
		
		return DepthStencilHandle((void*)CFBridgingRetain(result), MTLDeleter);
	}
	
	ShaderFunctionHandle MetalDevice::createFunction(std::string_view name) {
		id<MTLFunction> function = [shaderLib newFunctionWithName:[NSString stringWithUTF8String:name.data()]];
		return ShaderFunctionHandle((void*)CFBridgingRetain(function), MTLDeleter);
	}
	
	RenderPipelineHandle MetalDevice::createRenderPipeline(RenderPipelineDescription const& desc) {
		MTLRenderPipelineDescriptor* mtlDesc = [[MTLRenderPipelineDescriptor alloc] init];
		
        for (size_t index = 0; auto const& caDesc: desc.colorAttachments) {
			MTLRenderPipelineColorAttachmentDescriptor* mtlCADesc = [[MTLRenderPipelineColorAttachmentDescriptor alloc] init];
			mtlCADesc.pixelFormat = (MTLPixelFormat)caDesc.pixelFormat;
			mtlCADesc.blendingEnabled = caDesc.blendingEnabled;

			mtlCADesc.sourceRGBBlendFactor      = (MTLBlendFactor)caDesc.sourceRGBBlendFactor;
			mtlCADesc.destinationRGBBlendFactor = (MTLBlendFactor)caDesc.destinationRGBBlendFactor;
			mtlCADesc.rgbBlendOperation         = (MTLBlendOperation)caDesc.rgbBlendOperation;

			
			mtlCADesc.sourceAlphaBlendFactor      = (MTLBlendFactor)caDesc.sourceAlphaBlendFactor;
			mtlCADesc.destinationAlphaBlendFactor = (MTLBlendFactor)caDesc.destinationAlphaBlendFactor;
			mtlCADesc.alphaBlendOperation         = (MTLBlendOperation)caDesc.alphaBlendOperation;

			[mtlDesc.colorAttachments setObject: mtlCADesc
							 atIndexedSubscript: index];
            ++index;
		}
		
		mtlDesc.depthAttachmentPixelFormat = (MTLPixelFormat)desc.depthAttachmentPixelFormat;
		mtlDesc.stencilAttachmentPixelFormat = (MTLPixelFormat)desc.stencilAttachmentPixelFormat;
		
		mtlDesc.rasterSampleCount = desc.rasterSampleCount;
		mtlDesc.inputPrimitiveTopology = (MTLPrimitiveTopologyClass)desc.inputPrimitiveTopology;
		
		[mtlDesc setVertexFunction: (__bridge id<MTLFunction>)desc.vertexFunction.nativeHandle()];
		[mtlDesc setFragmentFunction: (__bridge id<MTLFunction>)desc.fragmentFunction.nativeHandle()];
		
		id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor: mtlDesc error: nil];
		if (!pipelineState) {
			bloomLog(error, "Failed to create Pipeline State");
			bloomDebugbreak();
		}
		
		MTLRenderPipelineReflection* refl = [[MTLRenderPipelineReflection alloc] init];
		
//		MTLAutoreleasedRenderPipelineReflection
		
		id<MTLRenderPipelineState> ps2 = [device newRenderPipelineStateWithDescriptor: mtlDesc
																			  options: MTLPipelineOption{}
																		   reflection: &refl
																				error: nil];
//		refl.
		
		return RenderPipelineHandle((void*)CFBridgingRetain(pipelineState), MTLDeleter);
	}
	
	ComputePipelineHandle MetalDevice::createComputePipeline(ComputePipelineDescription const& desc) {
		id<MTLComputePipelineState> pipelineState = [device newComputePipelineStateWithFunction:(__bridge id<MTLFunction>)desc.computeFunction.nativeHandle()
																						  error:nil];
		
		if (!pipelineState) {
			bloomLog(error, "Failed to create Pipeline State");
			bloomDebugbreak();
		}
		auto result = ComputePipelineHandle((void*)CFBridgingRetain(pipelineState), MTLDeleter);
		
		result.maxTotalThreadsPerThreadgroup = pipelineState.maxTotalThreadsPerThreadgroup;
		result.threadExecutionWidth = pipelineState.threadExecutionWidth;
		result.staticThreadgroupMemoryLength = pipelineState.staticThreadgroupMemoryLength;
		
		return result;
	}
	
	void MetalDevice::fillManagedBuffer(BufferView buffer, void const* data, std::size_t size, std::size_t offset) {
		id<MTLBuffer> mtlBuffer = (__bridge id<MTLBuffer>)buffer.nativeHandle();
		char* const contents = (char*)[mtlBuffer contents];
		std::memcpy(contents + offset, data, size);
		
		[mtlBuffer didModifyRange: NSRange{ offset, size }];
	}
	
	void MetalDevice::reloadDefaultLibrary() {
		shaderLib = [device newLibraryWithFile: [NSString stringWithCString: (resourceDir() / "default.metallib").c_str()
																   encoding: NSUTF8StringEncoding]
										 error: nil];
	}
	
	void* MetalDevice::nativeHandle() {
		return (__bridge void*)device;
	}
	
}

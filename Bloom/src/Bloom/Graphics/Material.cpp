#include "Material.hpp"

#include "Bloom/Platform/Metal/MetalRenderContext.hpp"
#include <Metal/Metal.hpp>

namespace bloom {
	
	Material Material::makeDefaultMaterial(RenderContext* renderContext) {
		Material material;

		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		auto* lib = device->newDefaultLibrary();
		
		
		/* main pass */ {
			auto* psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA32Float);
	
#define USE_PCSS_TEST_SHADER 0

			auto* vertexFunction = lib->newFunction(bloom::makeNSString("mainPassVS"));
			
#if !USE_PCSS_TEST_SHADER
			auto* fragmentFunction = lib->newFunction(bloom::makeNSString("mainPassFS"));
#else
			auto* fragmentFunction = lib->newFunction(bloom::makeNSString("mainPassFS_PCSSTest"));
#endif
			
			
			bloomAssert(vertexFunction);
			bloomAssert(fragmentFunction);
			psDesc->setVertexFunction(vertexFunction);
			psDesc->setFragmentFunction(fragmentFunction);
			psDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc, &errors);
			assert(ps);
			assert(!errors);
			material.mainPass = bloom::RenderPipelineHandle(ps, MTLDeleter);
			
			psDesc->release();
		}
		
		/* outline pass */ {
			auto* psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatR8Unorm);
			psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("outlinePassVS")));
			psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("outlinePassFS")));

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc, &errors);
			assert(ps);
			assert(!errors);
			material.outlinePass = bloom::RenderPipelineHandle(ps, MTLDeleter);
			
			psDesc->release();
		}
		
		
		lib->release();
		
		return material;
	}
	
}



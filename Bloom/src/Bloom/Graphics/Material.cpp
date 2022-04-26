#include "Material.hpp"

#include "Bloom/Platform/Metal/MetalRenderContext.hpp"
#include <Metal/Metal.hpp>

namespace bloom {
	
	Reference<Material> Material::makeDefaultMaterial(RenderContext* renderContext) {
		auto material = std::make_shared<bloom::Material>();

		auto* device = dynamic_cast<bloom::MetalRenderContext*>(renderContext)->device();
		auto* lib = device->newDefaultLibrary();
		
		
		/* main pass */ {
			auto* psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatRGBA32Float);
			psDesc->colorAttachments()->object(1)->setPixelFormat(MTL::PixelFormatR32Uint);
			psDesc->colorAttachments()->object(2)->setPixelFormat(MTL::PixelFormatR8Unorm);
			psDesc->colorAttachments()->object(3)->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
			psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("mainPassEditorVS")));
			psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("mainPassEditorFS")));
			psDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc, &errors);
			assert(ps);
			
			material->mainPassEditor = bloom::RenderPipelineHandle(ps, MTLDeleter);
			
			psDesc->release();
			errors->release();
		}
		
		/* outline pass */ {
			auto* psDesc = MTL::RenderPipelineDescriptor::alloc()->init();
			psDesc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatR8Unorm);
			psDesc->setVertexFunction(lib->newFunction(bloom::makeNSString("outlinePassVS")));
			psDesc->setFragmentFunction(lib->newFunction(bloom::makeNSString("outlinePassFS")));

			NS::Error* errors;
			auto* ps = device->newRenderPipelineState(psDesc, &errors);
			assert(ps);
			
			material->outlinePass = bloom::RenderPipelineHandle(ps, MTLDeleter);
			
			psDesc->release();
			errors->release();
		}
		
		
		lib->release();
		
		return material;
	}
	
}



#include "Material.hpp"

#include "Bloom/GPU/HardwareDevice.hpp"

namespace bloom {
	
	Material Material::makeDefaultMaterial(HardwareDevice& device, Asset base) {
		Material material(base);

		RenderPipelineDescription desc;
		ColorAttachmentDescription ca;
		
		// Main Pass
		ca.pixelFormat = PixelFormat::RGBA32Float;
		desc.colorAttachments.push_back(ca);
		
		desc.vertexFunction = device.createFunction("mainPassVS");
		desc.fragmentFunction = device.createFunction("mainPassFS");
		
		desc.depthAttachmentPixelFormat = PixelFormat::Depth32Float;
		
		material.mainPass = device.createRenderPipeline(desc);
		
		// Outline Pass
		desc.colorAttachments[0].pixelFormat = PixelFormat::R8Unorm;
		
		desc.vertexFunction = device.createFunction("outlinePassVS");
		desc.fragmentFunction = device.createFunction("outlinePassFS");
		
		
		material.outlinePass = device.createRenderPipeline(desc);
		
		return material;
	}
	
}



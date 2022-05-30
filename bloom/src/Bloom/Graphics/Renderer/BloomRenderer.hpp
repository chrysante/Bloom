#pragma once

#include "Bloom/Core/Core.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"

#include "BloomFramebuffer.hpp"
#include "ShaderParameters.hpp"

#include <mtl/mtl.hpp>

namespace bloom {
	
	class HardwareDevice;
	class CommandQueue;
	
	struct BloomRenderParameters {
		bool enabled = true;
		bool physicallyCorrect = true;
		float intensity = 1;
		float threshold = 1;
		float knee      = 0.1;
		float clamp     = 100;
		float diffusion = 1;
		float contribution = 0.1;
	};
	
	class BLOOM_API BloomRenderer {
	public:
		void init(HardwareDevice&);
		void populateFramebuffer(HardwareDevice&, BloomFramebuffer&, mtl::usize2) const;
		
		void render(CommandQueue&,
					BloomFramebuffer&,
					TextureView rawColor,
					BufferView renderParameters,
					mtl::uint2 framebufferSize);
		
		BloomParameters makeShaderParameters() const;
		
		BloomRenderParameters getParameters() const { return settings; }
		void setParameters(BloomRenderParameters const&);
		
	private:
		ComputePipelineHandle prefilterPipeline;
		ComputePipelineHandle downsamplePipeline;
		ComputePipelineHandle upsamplePipeline;
		
		BloomRenderParameters settings;
	};
	
}

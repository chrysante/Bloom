#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"
#include "RenderPrimitives.hpp"

namespace bloom {
	
	class RenderContext;
	
	class BLOOM_API Material {
	public:
		static Reference<Material> makeDefaultMaterial(RenderContext*);
		
		
//	private:
		RenderPipelineHandle mainPassEditor;
		RenderPipelineHandle outlinePass;
		void* functionTable;
	};
	
}

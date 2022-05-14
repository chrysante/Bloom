#pragma once

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Reference.hpp"
#include "RenderPrimitives.hpp"
#include "Bloom/Assets/Asset.hpp"

namespace bloom {
	
	class RenderContext;
	
	class BLOOM_API Material {
	public:
		static Material makeDefaultMaterial(RenderContext*);
		
		utl::UUID assetID() const { return _id; }
		
//	private:
		utl::UUID _id;
		
		RenderPipelineHandle mainPass;
		RenderPipelineHandle outlinePass;
		TriangleCullMode cullMode = TriangleCullMode::back;
		void* functionTable;
	};
	
}

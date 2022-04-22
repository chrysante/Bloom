#pragma once

namespace bloom {
	
	enum class RenderAPI {
		openGL, vulkan, metal, direct3D
	};
	
	inline RenderAPI selectRenderAPI() {
		return RenderAPI::metal;
	}
	
}
